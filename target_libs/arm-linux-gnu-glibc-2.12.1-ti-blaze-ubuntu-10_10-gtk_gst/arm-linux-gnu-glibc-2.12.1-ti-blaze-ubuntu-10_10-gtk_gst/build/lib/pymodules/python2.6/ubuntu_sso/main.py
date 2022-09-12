# ubuntu_sso.main - main login handling interface
#
# Author: Natalia Bidart <natalia.bidart@canonical.com>
# Author: Alejandro J. Cura <alecu@canonical.com>
#
# Copyright 2009 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
"""Single Sign On login handler.

An utility which accepts requests for Ubuntu Single Sign On login over D-Bus.

The OAuth process is handled, including adding the OAuth access token to the
gnome keyring.

"""

import os
import re
import threading
import traceback
import urllib2

import dbus.service
import gobject

# Unable to import 'lazr.restfulclient.*'
# pylint: disable=F0401
from lazr.restfulclient.authorize import BasicHttpAuthorizer
from lazr.restfulclient.authorize.oauth import OAuthAuthorizer
from lazr.restfulclient.errors import HTTPError
from lazr.restfulclient.resource import ServiceRoot
# pylint: enable=F0401
from oauth import oauth

from ubuntu_sso import DBUS_IFACE_USER_NAME, DBUS_IFACE_CRED_NAME
from ubuntu_sso.keyring import Keyring, get_token_name, U1_APP_NAME
from ubuntu_sso.logger import setup_logging


# Disable the invalid name warning, as we have a lot of DBus style names
# pylint: disable=C0103


logger = setup_logging("ubuntu_sso.main")
PING_URL = "https://one.ubuntu.com/oauth/sso-finished-so-get-tokens/"
SERVICE_URL = "https://login.ubuntu.com/api/1.0"
NO_OP = lambda *args, **kwargs: None


class NoDefaultConfigError(Exception):
    """No default section in configuration file"""


class BadRealmError(Exception):
    """Realm must be a URL."""


class InvalidEmailError(Exception):
    """The email is not valid."""


class InvalidPasswordError(Exception):
    """The password is not valid.

    Must provide at least 8 characters, one upper case, one number.
    """


class RegistrationError(Exception):
    """The registration failed."""


class AuthenticationError(Exception):
    """The authentication failed."""


class EmailTokenError(Exception):
    """The email token is not valid."""


class ResetPasswordTokenError(Exception):
    """The token for password reset could not be generated."""


class NewPasswordError(Exception):
    """The new password could not be set."""


def keyring_store_credentials(app_name, credentials, callback, *cb_args):
    """Store the credentials in the keyring."""

    def _inner():
        """Store the credentials, and trigger the callback."""
        logger.info('keyring_store_credentials: app_name "%s".', app_name)
        Keyring(app_name).set_ubuntusso_attr(credentials)
        callback(*cb_args)

    gobject.idle_add(_inner)


def keyring_get_credentials(app_name):
    """Get the credentials from the keyring or None if not there."""
    creds = Keyring(app_name).get_ubuntusso_attr()
    logger.info('keyring_get_credentials: app_name "%s", resulting credentials'
                ' is not None? %r', app_name, creds is not None)
    return creds


class SSOLoginProcessor(object):
    """Login and register users using the Ubuntu Single Sign On service."""

    def __init__(self, sso_service_class=None):
        """Create a new SSO login processor."""
        if sso_service_class is None:
            self.sso_service_class = ServiceRoot
        else:
            self.sso_service_class = sso_service_class

        self.service_url = os.environ.get('USSOC_SERVICE_URL', SERVICE_URL)

    def _valid_email(self, email):
        """Validate the given email."""
        return email is not None and '@' in email

    def _valid_password(self, password):
        """Validate the given password."""
        res = (len(password) > 7 and  # at least 8 characters
               re.search('[A-Z]', password) and  # one upper case
               re.search('\d+', password))  # one number
        return res

    def _format_webservice_errors(self, errdict):
        """Turn each list of strings in the errdict into a LF separated str."""
        result = {}
        for k, v in errdict.iteritems():
            # workaround until bug #624955 is solved
            if isinstance(v, basestring):
                result[k] = v
            else:
                result[k] = "\n".join(v)
        return result

    def generate_captcha(self, filename):
        """Generate a captcha using the SSO service."""
        logger.debug('generate_captcha: requesting captcha, filename: %r',
                     filename)
        sso_service = self.sso_service_class(None, self.service_url)
        captcha = sso_service.captchas.new()

        # download captcha and save to 'filename'
        logger.debug('generate_captcha: server answered: %r', captcha)
        try:
            res = urllib2.urlopen(captcha['image_url'])
            with open(filename, 'wb') as f:
                f.write(res.read())
        except:
            msg = 'generate_captcha crashed while downloading the image.'
            logger.exception(msg)
            raise

        return captcha['captcha_id']

    def register_with_name(self, email, password, displayname,
                      captcha_id, captcha_solution):
        """Register a new user with 'email' and 'password'."""
        logger.debug('register_with_name: email: %r password: <hidden>, '
                     'displayname: %r, captcha_id: %r, captcha_solution: %r',
                     email, displayname, captcha_id, captcha_solution)
        sso_service = self.sso_service_class(None, self.service_url)
        if not self._valid_email(email):
            logger.error('register_with_name: InvalidEmailError for email: %r',
                         email)
            raise InvalidEmailError()
        if not self._valid_password(password):
            logger.error('register_with_name: InvalidPasswordError')
            raise InvalidPasswordError()

        result = sso_service.registrations.register(
                    email=email, password=password,
                    displayname=displayname,
                    captcha_id=captcha_id,
                    captcha_solution=captcha_solution)
        logger.info('register_with_name: email: %r result: %r', email, result)

        if result['status'] == 'error':
            errorsdict = self._format_webservice_errors(result['errors'])
            raise RegistrationError(errorsdict)
        elif result['status'] != 'ok':
            raise RegistrationError('Received unknown status: %s' % result)
        else:
            return email

    def register_user(self, email, password,
                      captcha_id, captcha_solution):
        """Register a new user with 'email' and 'password'."""
        logger.debug('register_user: email: %r password: <hidden>, '
                     'captcha_id: %r, captcha_solution: %r',
                     email, captcha_id, captcha_solution)
        res = self.register_with_name(email, password, displayname='',
                                      captcha_id=captcha_id,
                                      captcha_solution=captcha_solution)
        return res

    def login(self, email, password, token_name):
        """Login a user with 'email' and 'password'."""
        logger.debug('login: email: %r password: <hidden>, token_name: %r',
                     email, token_name)
        basic = BasicHttpAuthorizer(email, password)
        sso_service = self.sso_service_class(basic, self.service_url)
        service = sso_service.authentications.authenticate

        try:
            credentials = service(token_name=token_name)
        except HTTPError:
            logger.exception('login failed with:')
            raise AuthenticationError()

        logger.debug('login: authentication successful! consumer_key: %r, ' \
                     'token_name: %r', credentials['consumer_key'], token_name)
        return credentials

    def is_validated(self, token, sso_service=None):
        """Return if user with 'email' and 'password' is validated."""
        logger.debug('is_validated: requesting accounts.me() info.')
        if sso_service is None:
            oauth_token = oauth.OAuthToken(token['token'],
                                           token['token_secret'])
            authorizer = OAuthAuthorizer(token['consumer_key'],
                                         token['consumer_secret'],
                                         oauth_token)
            sso_service = self.sso_service_class(authorizer, self.service_url)

        me_info = sso_service.accounts.me()
        key = 'preferred_email'
        result = key in me_info and me_info[key] != None

        logger.info('is_validated: consumer_key: %r, result: %r.',
                    token['consumer_key'], result)
        return result

    def validate_email(self, email, password, email_token, token_name):
        """Validate an email token for user with 'email' and 'password'."""
        logger.debug('validate_email: email: %r password: <hidden>, '
                     'email_token: %r, token_name: %r.',
                     email, email_token, token_name)
        token = self.login(email=email, password=password,
                           token_name=token_name)

        oauth_token = oauth.OAuthToken(token['token'], token['token_secret'])
        authorizer = OAuthAuthorizer(token['consumer_key'],
                                     token['consumer_secret'],
                                     oauth_token)
        sso_service = self.sso_service_class(authorizer, self.service_url)
        result = sso_service.accounts.validate_email(email_token=email_token)
        logger.info('validate_email: email: %r result: %r', email, result)
        if 'errors' in result:
            errorsdict = self._format_webservice_errors(result['errors'])
            raise EmailTokenError(errorsdict)
        elif 'email' in result:
            return token
        else:
            raise EmailTokenError('Received invalid reply: %s' % result)

    def request_password_reset_token(self, email):
        """Request a token to reset the password for the account 'email'."""
        sso_service = self.sso_service_class(None, self.service_url)
        service = sso_service.registrations.request_password_reset_token
        try:
            result = service(email=email)
        except HTTPError, e:
            logger.exception('request_password_reset_token failed with:')
            raise ResetPasswordTokenError(e.content.split('\n')[0])

        if result['status'] == 'ok':
            return email
        else:
            raise ResetPasswordTokenError('Received invalid reply: %s' %
                                          result)

    def set_new_password(self, email, token, new_password):
        """Set a new password for the account 'email' to be 'new_password'.

        The 'token' has to be the one resulting from a call to
        'request_password_reset_token'.

        """
        sso_service = self.sso_service_class(None, self.service_url)
        service = sso_service.registrations.set_new_password
        try:
            result = service(email=email, token=token,
                             new_password=new_password)
        except HTTPError, e:
            logger.exception('set_new_password failed with:')
            raise NewPasswordError(e.content.split('\n')[0])

        if result['status'] == 'ok':
            return email
        else:
            raise NewPasswordError('Received invalid reply: %s' % result)


def except_to_errdict(e):
    """Turn an exception into a dictionary to return thru DBus."""
    result = {
        "errtype": e.__class__.__name__,
    }
    if len(e.args) == 0:
        result["message"] = e.__class__.__doc__
    elif isinstance(e.args[0], dict):
        result.update(e.args[0])
    elif isinstance(e.args[0], basestring):
        result["message"] = e.args[0]

    return result


def blocking(f, app_name, result_cb, error_cb):
    """Run f in a thread; return or throw an exception thru the callbacks."""
    def _in_thread():
        """The part that runs inside the thread."""
        try:
            result_cb(app_name, f())
        except Exception, e:  # pylint: disable=W0703
            msg = "Exception while running DBus blocking code in a thread:"
            logger.exception(msg)
            error_cb(app_name, except_to_errdict(e))
    threading.Thread(target=_in_thread).start()


class SSOLogin(dbus.service.Object):
    """Login thru the Single Sign On service."""

    # Operator not preceded by a space (fails with dbus decorators)
    # pylint: disable=C0322

    def __init__(self, bus_name,
                 sso_login_processor_class=SSOLoginProcessor,
                 sso_service_class=None):
        """Initiate the Login object."""
        dbus.service.Object.__init__(self, object_path="/sso",
                                     bus_name=bus_name)
        self.sso_login_processor_class = sso_login_processor_class
        self.processor = self.sso_login_processor_class(
                                    sso_service_class=sso_service_class)

    # generate_capcha signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def CaptchaGenerated(self, app_name, result):
        """Signal thrown after the captcha is generated."""
        logger.debug('SSOLogin: emitting CaptchaGenerated with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def CaptchaGenerationError(self, app_name, error):
        """Signal thrown when there's a problem generating the captcha."""
        logger.debug('SSOLogin: emitting CaptchaGenerationError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='ss')
    def generate_captcha(self, app_name, filename):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            return self.processor.generate_captcha(filename)
        blocking(f, app_name, self.CaptchaGenerated,
                 self.CaptchaGenerationError)

    # register_user signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def UserRegistered(self, app_name, result):
        """Signal thrown when the user is registered."""
        logger.debug('SSOLogin: emitting UserRegistered with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def UserRegistrationError(self, app_name, error):
        """Signal thrown when there's a problem registering the user."""
        logger.debug('SSOLogin: emitting UserRegistrationError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='sssss')
    def register_user(self, app_name, email, password,
                      captcha_id, captcha_solution):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            return self.processor.register_user(email, password,
                                                captcha_id, captcha_solution)
        blocking(f, app_name, self.UserRegistered, self.UserRegistrationError)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='ssssss')
    def register_with_name(self, app_name, email, password, name,
                           captcha_id, captcha_solution):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            return self.processor.register_with_name(email, password, name,
                                                captcha_id, captcha_solution)
        blocking(f, app_name, self.UserRegistered, self.UserRegistrationError)

    # login signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def LoggedIn(self, app_name, result):
        """Signal thrown when the user is logged in."""
        logger.debug('SSOLogin: emitting LoggedIn with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def LoginError(self, app_name, error):
        """Signal thrown when there is a problem in the login."""
        logger.debug('SSOLogin: emitting LoginError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def UserNotValidated(self, app_name, result):
        """Signal thrown when the user is not validated."""
        logger.debug('SSOLogin: emitting UserNotValidated with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='sss')
    def login(self, app_name, email, password):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            token_name = get_token_name(app_name)
            logger.debug('login: token_name %r, email %r, password <hidden>.',
                         token_name, email)
            credentials = self.processor.login(email, password, token_name)
            logger.debug('login returned not None credentials? %r.',
                         credentials is not None)
            return credentials

        def success_cb(app_name, credentials):
            """Login finished successfull."""
            is_validated = self.processor.is_validated(credentials)
            logger.debug('user is validated? %r.', is_validated)
            if is_validated:
                keyring_store_credentials(app_name, credentials,
                                          self.LoggedIn, app_name, email)
            else:
                self.UserNotValidated(app_name, email)
        blocking(f, app_name, success_cb, self.LoginError)

    # validate_email signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def EmailValidated(self, app_name, result):
        """Signal thrown after the email is validated."""
        logger.debug('SSOLogin: emitting EmailValidated with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def EmailValidationError(self, app_name, error):
        """Signal thrown when there's a problem validating the email."""
        logger.debug('SSOLogin: emitting EmailValidationError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='ssss')
    def validate_email(self, app_name, email, password, email_token):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            token_name = get_token_name(app_name)
            credentials = self.processor.validate_email(email, password,
                                                      email_token, token_name)

            def _email_stored():
                """The email was stored, so call the signal."""
                self.EmailValidated(app_name, email)

            keyring_store_credentials(app_name, credentials, _email_stored)

        blocking(f, app_name, NO_OP, self.EmailValidationError)

    # request_password_reset_token signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def PasswordResetTokenSent(self, app_name, result):
        """Signal thrown when the token is succesfully sent."""
        logger.debug('SSOLogin: emitting PasswordResetTokenSent with app_name '
                     '"%s" and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def PasswordResetError(self, app_name, error):
        """Signal thrown when there's a problem sending the token."""
        logger.debug('SSOLogin: emitting PasswordResetError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='ss')
    def request_password_reset_token(self, app_name, email):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            return self.processor.request_password_reset_token(email)
        blocking(f, app_name, self.PasswordResetTokenSent,
                 self.PasswordResetError)

    # set_new_password signals
    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="ss")
    def PasswordChanged(self, app_name, result):
        """Signal thrown when the token is succesfully sent."""
        logger.debug('SSOLogin: emitting PasswordChanged with app_name "%s" '
                     'and result %r', app_name, result)

    @dbus.service.signal(DBUS_IFACE_USER_NAME, signature="sa{ss}")
    def PasswordChangeError(self, app_name, error):
        """Signal thrown when there's a problem sending the token."""
        logger.debug('SSOLogin: emitting PasswordChangeError with '
                     'app_name "%s" and error %r', app_name, error)

    @dbus.service.method(dbus_interface=DBUS_IFACE_USER_NAME,
                         in_signature='ssss')
    def set_new_password(self, app_name, email, token, new_password):
        """Call the matching method in the processor."""
        def f():
            """Inner function that will be run in a thread."""
            return self.processor.set_new_password(email, token,
                                                   new_password)
        blocking(f, app_name, self.PasswordChanged, self.PasswordChangeError)


class SSOCredentials(dbus.service.Object):
    """DBus object that gets credentials, and login/registers if needed."""

    # Operator not preceded by a space (fails with dbus decorators)
    # pylint: disable=C0322

    def __init__(self, *args, **kwargs):
        dbus.service.Object.__init__(self, *args, **kwargs)
        self.ping_url = os.environ.get('USSOC_PING_URL', PING_URL)

    @dbus.service.signal(DBUS_IFACE_CRED_NAME, signature="s")
    def AuthorizationDenied(self, app_name):
        """Signal thrown when the user denies the authorization."""
        logger.info('SSOCredentials: emitting AuthorizationDenied with '
                    'app_name "%s"', app_name)

    @dbus.service.signal(DBUS_IFACE_CRED_NAME, signature="sa{ss}")
    def CredentialsFound(self, app_name, credentials):
        """Signal thrown when the credentials are found."""
        logger.info('SSOCredentials: emitting CredentialsFound with '
                    'app_name "%s"', app_name)

    @dbus.service.signal(DBUS_IFACE_CRED_NAME, signature="sss")
    def CredentialsError(self, app_name, error_message, detailed_error):
        """Signal thrown when there is a problem finding the credentials."""
        logger.error('SSOCredentials: emitting CredentialsError with app_name '
                     '"%s" and error_message %r', app_name, error_message)

    @dbus.service.method(dbus_interface=DBUS_IFACE_CRED_NAME,
                         in_signature="s", out_signature="a{ss}")
    def find_credentials(self, app_name):
        """Get the credentials from the keyring or '' if not there."""
        token = keyring_get_credentials(app_name)
        logger.info('find_credentials: app_name "%s", result is {}? %s',
                    app_name, token is None)
        if token is None:
            return {}
        else:
            return token

    def _login_success_cb(self, dialog, app_name, email):
        """Handles the response from the UI dialog."""
        logger.info('Login successful for app %r, email %r. Still pending to '
                    'ping server and send result signal.', app_name, email)
        try:
            creds = keyring_get_credentials(app_name)
            self._ping_url(app_name, email, creds)
            self.CredentialsFound(app_name, creds)
        except:  # pylint: disable=W0702
            msg = "Problem getting the credentials from the keyring."
            logger.exception(msg)
            self.clear_token(app_name)
            self.CredentialsError(app_name, msg, traceback.format_exc())

    def _login_error_cb(self, dialog, app_name, error):
        """Handles a problem in the UI."""
        logger.info('Login unsuccessful for app %r, error %r', app_name, error)
        msg = "Problem getting the credentials from the keyring."
        self.CredentialsError(app_name, msg, "no more info available")

    def _login_auth_denied_cb(self, dialog, app_name):
        """The user decides not to allow the registration or login."""
        self.AuthorizationDenied(app_name)

    def _ping_url(self, app_name, email, credentials):
        """Ping the given url."""
        logger.info('Maybe pinging server for app_name "%s"', app_name)
        if app_name == U1_APP_NAME:
            url = self.ping_url + email
            consumer = oauth.OAuthConsumer(credentials['consumer_key'],
                                           credentials['consumer_secret'])
            token = oauth.OAuthToken(credentials['token'],
                                     credentials['token_secret'])
            get_request = oauth.OAuthRequest.from_consumer_and_token
            oauth_req = get_request(oauth_consumer=consumer, token=token,
                                    http_method="GET", http_url=url)
            oauth_req.sign_request(oauth.OAuthSignatureMethod_HMAC_SHA1(),
                                   consumer, token)
            request = urllib2.Request(url, headers=oauth_req.to_header())
            logger.debug('Opening the ping url %s with urllib2.urlopen. ' \
                         'Request to: %s', PING_URL, request.get_full_url())
            response = urllib2.urlopen(request)
            logger.debug('Url opened. Response: %s.', response.code)
            return response.code

    def _show_login_or_register_ui(self, app_name, tc_url, help_text,
                                   win_id, login_only=False):
        """Shows the UI so the user can login or register."""
        try:
            # delay gui import to be able to function on non-graphical envs
            from ubuntu_sso import gui
            gui_app = gui.UbuntuSSOClientGUI(app_name, tc_url,
                                             help_text, win_id, login_only)
            gui_app.connect(gui.SIG_LOGIN_SUCCEEDED, self._login_success_cb)
            gui_app.connect(gui.SIG_LOGIN_FAILED, self._login_error_cb)
            gui_app.connect(gui.SIG_REGISTRATION_SUCCEEDED,
                            self._login_success_cb)
            gui_app.connect(gui.SIG_REGISTRATION_FAILED, self._login_error_cb)
            gui_app.connect(gui.SIG_USER_CANCELATION,
                            self._login_auth_denied_cb)
        except:  # pylint: disable=W0702
            msg = '_show_login_or_register_ui failed when calling ' \
                  'gui.UbuntuSSOClientGUI(%r, %r, %r, %r, %r)'
            logger.exception(msg, app_name, tc_url, help_text,
                             win_id, login_only)
            msg = "Problem opening the Ubuntu SSO user interface."
            self.CredentialsError(app_name, msg, traceback.format_exc())

    @dbus.service.method(dbus_interface=DBUS_IFACE_CRED_NAME,
                         in_signature="sssx", out_signature="")
    def login_or_register_to_get_credentials(self, app_name,
                                             terms_and_conditions_url,
                                             help_text, window_id):
        """Get credentials if found else prompt GUI to login or register.

        'app_name' will be displayed in the GUI.
        'terms_and_conditions_url' will be the URL pointing to T&C.
        'help_text' is an explanatory text for the end-users, will be shown
         below the headers.
        'window_id' is the id of the window which will be set as a parent of
         the GUI. If 0, no parent will be set.

        """
        try:
            token = keyring_get_credentials(app_name)
            if token is None:
                gobject.idle_add(self._show_login_or_register_ui,
                                 app_name, terms_and_conditions_url,
                                 help_text, window_id)
            else:
                self.CredentialsFound(app_name, token)
        except:  # pylint: disable=W0702
            msg = "Problem getting the credentials from the keyring."
            logger.exception(msg)
            self.CredentialsError(app_name, msg, traceback.format_exc())

    def _show_login_only_ui(self, app_name, help_text, win_id):
        """Shows the UI so the user can login."""
        tc_url = None  # no terms and conditions when only logging in
        self._show_login_or_register_ui(app_name, tc_url, help_text,
                                        win_id, login_only=True)

    @dbus.service.method(dbus_interface=DBUS_IFACE_CRED_NAME,
                         in_signature="ssx", out_signature="")
    def login_to_get_credentials(self, app_name, help_text, window_id):
        """Get credentials if found else prompt GUI just to login

        'app_name' will be displayed in the GUI.
        'help_text' is an explanatory text for the end-users, will be shown
         before the login fields.
        'window_id' is the id of the window which will be set as a parent of
         the GUI. If 0, no parent will be set.

        """
        try:
            token = keyring_get_credentials(app_name)
            if token is None:
                gobject.idle_add(self._show_login_only_ui, app_name,
                                 help_text, window_id)
            else:
                self.CredentialsFound(app_name, token)
        except:  # pylint: disable=W0702
            msg = "Problem getting the credentials from the keyring."
            logger.exception(msg)
            self.CredentialsError(app_name, msg, traceback.format_exc())

    @dbus.service.method(dbus_interface=DBUS_IFACE_CRED_NAME,
                         in_signature='s', out_signature='')
    def clear_token(self, app_name):
        """Clear the token for an application from the keyring.

        'app_name' is the name of the application.
        """
        logger.info('Clearing credentials for app %r.', app_name)
        try:
            creds = Keyring(app_name)
            creds.delete_ubuntusso_attr()
        except:  # pylint: disable=W0702
            logger.exception(
                "problem removing credentials from keyring for %s",
                app_name)
