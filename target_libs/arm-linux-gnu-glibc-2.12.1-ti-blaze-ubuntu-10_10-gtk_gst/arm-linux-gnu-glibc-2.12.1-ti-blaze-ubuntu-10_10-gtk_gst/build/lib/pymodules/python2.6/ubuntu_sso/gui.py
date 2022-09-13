# ubuntu_sso.gui - GUI for login and registration
#
# Author: Natalia Bidart <natalia.bidart@canonical.com>
#
# Copyright 2010 Canonical Ltd.
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

"""User registration GUI."""

import logging
import os
import re
import tempfile

from functools import wraps

import dbus
import gettext
import gobject
import gtk  # pylint: disable=W0403
import webkit
import xdg

from dbus.mainloop.glib import DBusGMainLoop

from ubuntu_sso import DBUS_PATH, DBUS_BUS_NAME, DBUS_IFACE_USER_NAME
from ubuntu_sso.logger import setup_logging


# Instance of 'UbuntuSSOClientGUI' has no 'yyy' member
# pylint: disable=E1101


_ = gettext.gettext
gettext.textdomain('ubuntu-sso-client')

DBusGMainLoop(set_as_default=True)
logger = setup_logging('ubuntu_sso.gui')


NO_OP = lambda *args, **kwargs: None
DEFAULT_WIDTH = 30
# To be replaced by values from the theme (LP: #616526)
HELP_TEXT_COLOR = gtk.gdk.Color("#bfbfbf")
WARNING_TEXT_COLOR = gtk.gdk.Color("red")

SIG_LOGIN_FAILED = 'login-failed'
SIG_LOGIN_SUCCEEDED = 'login-succeeded'
SIG_REGISTRATION_FAILED = 'registration-failed'
SIG_REGISTRATION_SUCCEEDED = 'registration-succeeded'
SIG_USER_CANCELATION = 'user-cancelation'

SIGNAL_ARGUMENTS = [
    (SIG_LOGIN_FAILED, (gobject.TYPE_STRING, gobject.TYPE_STRING)),
    (SIG_LOGIN_SUCCEEDED, (gobject.TYPE_STRING, gobject.TYPE_STRING,)),
    (SIG_REGISTRATION_FAILED, (gobject.TYPE_STRING, gobject.TYPE_STRING)),
    (SIG_REGISTRATION_SUCCEEDED, (gobject.TYPE_STRING, gobject.TYPE_STRING,)),
    (SIG_USER_CANCELATION, (gobject.TYPE_STRING,)),
]

for sig, sig_args in SIGNAL_ARGUMENTS:
    gobject.signal_new(sig, gtk.Window, gobject.SIGNAL_RUN_FIRST,
                       gobject.TYPE_NONE, sig_args)


def get_data_dir():
    """Build absolute path to  the 'data' directory."""
    module = os.path.dirname(__file__)
    result = os.path.abspath(os.path.join(module, os.pardir, 'data'))
    logger.debug('get_data_file: trying to load from %r (exists? %s)',
                 result, os.path.exists(result))
    if os.path.exists(result):
        logger.info('get_data_file: returning data dir located at %r.', result)
        return result

    # no local data dir, looking within system data dirs
    data_dirs = xdg.BaseDirectory.xdg_data_dirs
    for path in data_dirs:
        result = os.path.join(path, 'ubuntu-sso-client', 'data')
        result = os.path.abspath(result)
        logger.debug('get_data_file: trying to load from %r (exists? %s)',
                     result, os.path.exists(result))
        if os.path.exists(result):
            logger.info('get_data_file: data dir located at %r.', result)
            return result
    else:
        msg = 'get_data_file: can not build a valid data path. Giving up.' \
              '__file__ is %r, data_dirs are %r'
        logger.error(msg, __file__, data_dirs)


def get_data_file(filename):
    """Build absolute path to 'filename' within the 'data' directory."""
    return os.path.join(get_data_dir(), filename)


def log_call(f):
    """Decorator to log call funtions."""

    @wraps(f)
    def inner(*args, **kwargs):
        """Execute 'f' logging the call as INFO."""
        logger.info('%s: args %r, kwargs %r.', f.__name__, args, kwargs)
        return f(*args, **kwargs)

    return inner


class LabeledEntry(gtk.Entry):
    """An entry that displays the label within itself ina grey color."""

    def __init__(self, label, is_password=False, *args, **kwargs):
        self.label = label
        self.is_password = is_password
        self.warning = None

        gtk.Entry.__init__(self, *args, **kwargs)

        self.set_width_chars(DEFAULT_WIDTH)
        self._set_label(self, None)
        self.set_tooltip_text(self.label)
        self.connect('focus-in-event', self._clear_text)
        self.connect('focus-out-event', self._set_label)
        self.clear_warning()
        self.show()

    def _clear_text(self, *args, **kwargs):
        """Clear text and restore text color."""
        self.set_text(self.get_text())

        self.modify_text(gtk.STATE_NORMAL, None)  # restore to theme's default

        if self.is_password:
            self.set_visibility(False)

        return False  # propagate the event further

    def _set_label(self, *args, **kwargs):
        """Set the proper label and proper coloring."""
        if self.get_text():
            return

        self.set_text(self.label)
        self.modify_text(gtk.STATE_NORMAL, HELP_TEXT_COLOR)

        if self.is_password:
            self.set_visibility(True)

        return False  # propagate the event further

    def get_text(self):
        """Get text only if it's not the label nor empty."""
        result = gtk.Entry.get_text(self)
        if result == self.label or result.isspace():
            result = ''
        return result

    def set_warning(self, warning_msg):
        """Display warning as secondary icon, set 'warning_msg' as tooltip."""
        self.warning = warning_msg
        self.set_property('secondary-icon-stock', gtk.STOCK_DIALOG_WARNING)
        self.set_property('secondary-icon-sensitive', True)
        self.set_property('secondary-icon-activatable', False)
        self.set_property('secondary-icon-tooltip-text', warning_msg)

    def clear_warning(self):
        """Remove any warning."""
        self.warning = None
        self.set_property('secondary-icon-stock', None)
        self.set_property('secondary-icon-sensitive', False)
        self.set_property('secondary-icon-activatable', False)
        self.set_property('secondary-icon-tooltip-text', None)


class UbuntuSSOClientGUI(object):
    """Ubuntu single sign on GUI."""

    CAPTCHA_SOLUTION_ENTRY = _('Type the characters above')
    CONNECT_HELP_LABEL = _('To connect this computer to %(app_name)s ' \
                           'enter your details below.')
    EMAIL1_ENTRY = _('Email address')
    EMAIL2_ENTRY = _('Re-type Email address')
    EMAIL_MISMATCH = _('The email addresses don\'t match, please double check '
                       'and try entering them again.')
    EMAIL_INVALID = _('The email must be a valid email address.')
    EMAIL_TOKEN_ENTRY = _('Enter code verification here')
    FIELD_REQUIRED = _('This field is required.')
    FORGOTTEN_PASSWORD_BUTTON = _('I\'ve forgotten my password')
    JOIN_HEADER_LABEL = _('Create %(app_name)s account')
    LOADING = _('Loading...')
    LOGIN_BUTTON_LABEL = _('Already have an account? Click here to sign in')
    LOGIN_EMAIL_ENTRY = _('Email address')
    LOGIN_HEADER_LABEL = _('Connect to %(app_name)s')
    LOGIN_PASSWORD_ENTRY = _('Password')
    NAME_ENTRY = _('Name')
    NEXT = _('Next')
    ONE_MOMENT_PLEASE = _('One moment please...')
    PASSWORD_CHANGED = _('Your password was successfully changed.')
    PASSWORD1_ENTRY = RESET_PASSWORD1_ENTRY = _('Password')
    PASSWORD2_ENTRY = RESET_PASSWORD2_ENTRY = _('Re-type Password')
    PASSWORD_HELP = _('The password must have a minimum of 8 characters and ' \
                      'include one uppercase character and one number.')
    PASSWORD_MISMATCH = _('The passwords don\'t match, please double check ' \
                          'and try entering them again.')
    PASSWORD_TOO_WEAK = _('The password is too weak.')
    REQUEST_PASSWORD_TOKEN_LABEL = _('To reset your %(app_name)s password,'
                                     ' enter your email address below:')
    RESET_PASSWORD = _('Reset password')
    RESET_CODE_ENTRY = _('Reset code')
    RESET_EMAIL_ENTRY = _('Email address')
    SET_NEW_PASSWORD_LABEL = _('A password reset code has been sent to ' \
                               '%(email)s.\nPlease enter the code below ' \
                               'along with your new password.')
    SUCCESS = _('The process finished successfully. Congratulations!')
    TC = _('Terms & Conditions')
    TC_NOT_ACCEPTED = _('Agreeing to the Ubuntu One Terms & Conditions is ' \
                        'required to subscribe.')
    UNKNOWN_ERROR = _('There was an error when trying to complete the ' \
                      'process. Please check the information and try again.')
    VERIFY_EMAIL_LABEL = ('<b>%s</b>\n\n' % _('Enter verification code') +
                          _('Check %(email)s for an email from'
                            ' Ubuntu Single Sign On.'
                            ' This message contains a verification code.'
                            ' Enter the code in the field below and click OK'
                            ' to complete creating your %(app_name)s account'))
    YES_TO_TC = _('I agree with the %(app_name)s ')  # Terms&Conditions button
    YES_TO_UPDATES = _('Yes! Email me %(app_name)s tips and updates.')
    CAPTCHA_RELOAD_TOOLTIP = _('Reload')

    def __init__(self, app_name, tc_uri, help_text,
                 window_id=0, login_only=False, close_callback=None):
        """Create the GUI and initialize widgets."""
        gtk.link_button_set_uri_hook(NO_OP)

        self._captcha_filename = tempfile.mktemp()
        self._captcha_id = None
        self._signals_receivers = {}

        self.app_name = app_name
        self.app_label = '<b>%s</b>' % self.app_name
        self.tc_uri = tc_uri
        self.help_text = help_text
        self.close_callback = close_callback
        self.user_email = None
        self.user_password = None

        ui_filename = get_data_file('ui.glade')
        builder = gtk.Builder()
        builder.add_from_file(ui_filename)
        builder.connect_signals(self)

        self.widgets = []
        self.warnings = []
        self.cancels = []
        self.labels = []
        for obj in builder.get_objects():
            name = getattr(obj, 'name', None)
            if name is None and isinstance(obj, gtk.Buildable):
                # work around bug lp:507739
                name = gtk.Buildable.get_name(obj)
            if name is None:
                logging.warn("%s has no name (??)", obj)
            else:
                self.widgets.append(name)
                setattr(self, name, obj)
                if 'warning' in name:
                    self.warnings.append(obj)
                    obj.hide()
                if 'cancel_button' in name:
                    obj.connect('clicked', self.on_close_clicked)
                    self.cancels.append(obj)
                if '_button' in name:
                    obj.connect('activate', lambda w: w.clicked())
                if 'label' in name:
                    self.labels.append(obj)

        self.entries = (u'name_entry', u'email1_entry', u'email2_entry',
                        u'password1_entry', u'password2_entry',
                        u'captcha_solution_entry', u'email_token_entry',
                        u'login_email_entry', u'login_password_entry',
                        u'reset_email_entry', u'reset_code_entry',
                        u'reset_password1_entry', u'reset_password2_entry')

        for name in self.entries:
            label = getattr(self, name.upper())
            is_password = 'password' in name
            entry = LabeledEntry(label=label, is_password=is_password)
            setattr(self, name, entry)
            assert getattr(self, name) is not None

        self.window.set_icon_name('ubuntu-logo')

        self.bus = dbus.SessionBus()
        obj = self.bus.get_object(bus_name=DBUS_BUS_NAME,
                                  object_path=DBUS_PATH,
                                  follow_name_owner_changes=True)
        self.iface_name = DBUS_IFACE_USER_NAME
        self.backend = dbus.Interface(object=obj,
                                      dbus_interface=self.iface_name)
        logger.debug('UbuntuSSOClientGUI: backend created: %r', self.backend)

        self.pages = (self.enter_details_vbox, self.processing_vbox,
                      self.verify_email_vbox, self.success_vbox,
                      self.tc_browser_vbox, self.login_vbox,
                      self.request_password_token_vbox,
                      self.set_new_password_vbox)

        self._append_page(self._build_processing_page())
        self._append_page(self._build_success_page())
        self._append_page(self._build_login_page())
        self._append_page(self._build_request_password_token_page())
        self._append_page(self._build_set_new_password_page())
        self._append_page(self._build_verify_email_page())

        window_size = None
        if not login_only:
            window_size = (550, 500)
            self._append_page(self._build_enter_details_page())
            self._append_page(self._build_tc_page())
            self.login_button.grab_focus()
            self._set_current_page(self.enter_details_vbox)
        else:
            window_size = (400, 350)
            self.login_back_button.hide()
            self.login_ok_button.grab_focus()
            self.login_vbox.help_text = help_text
            self._set_current_page(self.login_vbox)

        self.window.set_size_request(*window_size)
        size_req = (int(window_size[0] * 0.9), -1)
        for label in self.labels:
            label.set_size_request(*size_req)

        self._signals = {
            'CaptchaGenerated':
             self._filter_by_app_name(self.on_captcha_generated),
            'CaptchaGenerationError':
             self._filter_by_app_name(self.on_captcha_generation_error),
            'UserRegistered':
             self._filter_by_app_name(self.on_user_registered),
            'UserRegistrationError':
             self._filter_by_app_name(self.on_user_registration_error),
            'EmailValidated':
             self._filter_by_app_name(self.on_email_validated),
            'EmailValidationError':
             self._filter_by_app_name(self.on_email_validation_error),
            'LoggedIn':
             self._filter_by_app_name(self.on_logged_in),
            'LoginError':
             self._filter_by_app_name(self.on_login_error),
            'UserNotValidated':
             self._filter_by_app_name(self.on_user_not_validated),
            'PasswordResetTokenSent':
             self._filter_by_app_name(self.on_password_reset_token_sent),
            'PasswordResetError':
             self._filter_by_app_name(self.on_password_reset_error),
            'PasswordChanged':
             self._filter_by_app_name(self.on_password_changed),
            'PasswordChangeError':
             self._filter_by_app_name(self.on_password_change_error),
        }
        self._setup_signals()
        self._gtk_signal_log = []

        if window_id != 0:
            # be as robust as possible:
            # if the window_id is not "good", set_transient_for will fail
            # awfully, and we don't want that: if the window_id is bad we can
            # still do everything as a standalone window. Also,
            # window_foreign_new may return None breaking set_transient_for.
            try:
                win = gtk.gdk.window_foreign_new(window_id)
                self.window.realize()
                self.window.window.set_transient_for(win)
            except:  # pylint: disable=W0702
                msg = 'UbuntuSSOClientGUI: failed set_transient_for win id %r'
                logger.exception(msg, window_id)

        self.yes_to_updates_checkbutton.hide()

        self.window.show()

    def _filter_by_app_name(self, f):
        """Excecute the decorated function only for 'self.app_name'."""

        @wraps(f)
        def inner(app_name, *args, **kwargs):
            """Execute 'f' only if 'app_name' matches 'self.app_name'."""
            result = None
            if app_name == self.app_name:
                result = f(app_name, *args, **kwargs)
            else:
                logger.info('%s: ignoring call since received app_name '\
                            '"%s" (expected "%s")',
                            f.__name__, app_name, self.app_name)
            return result

        return inner

    def _setup_signals(self):
        """Bind signals to callbacks to be able to test the pages."""
        iface = self.iface_name
        for signal, method in self._signals.iteritems():
            actual = self._signals_receivers.get((iface, signal))
            if actual is not None:
                msg = 'Signal %r is already connected with %r at iface %r.'
                logger.warning(msg, signal, actual, iface)

            match = self.bus.add_signal_receiver(method, signal_name=signal,
                                                 dbus_interface=iface)
            logger.debug('Connecting signal %r with method %r at iface %r.' \
                         'Match: %r', signal, method, iface, match)
            self._signals_receivers[(iface, signal)] = method

    def _debug(self, *args, **kwargs):
        """Do some debugging."""
        print args, kwargs

    def _add_spinner_to_container(self, container, legend=None):
        """Add a spinner to 'container'."""
        spinner = gtk.Spinner()
        spinner.start()

        label = gtk.Label()
        if legend:
            label.set_text(legend)
        else:
            label.set_text(self.LOADING)

        hbox = gtk.HBox(spacing=5)
        hbox.pack_start(spinner, expand=False)
        hbox.pack_start(label, expand=False)

        alignment = gtk.Alignment(xalign=0.5, yalign=0.5)
        alignment.add(hbox)
        alignment.show_all()

        container.add(alignment)

    # helpers

    def _set_warning_message(self, widget, message):
        """Set 'message' as text for 'widget'."""
        widget.set_text(message)
        widget.modify_fg(gtk.STATE_NORMAL, WARNING_TEXT_COLOR)
        widget.show()

    def _clear_warnings(self):
        """Clear all warning messages."""
        for widget in self.warnings:
            widget.set_text('')
            widget.hide()
        for widget in self.entries:
            getattr(self, widget).clear_warning()

    def _non_empty_input(self, widget):
        """Return weather widget has non empty content."""
        text = widget.get_text()
        return bool(text and not text.isspace())

    # build pages

    def _append_page(self, page):
        """Append 'page' to the 'window'."""
        self.window.get_children()[0].pack_start(page)

    def _set_header(self, header):
        """Set 'header' as the window title and header."""
        markup = '<span size="xx-large">%s</span>'
        self.header_label.set_markup(markup % header)
        self.window.set_title(self.header_label.get_text())  # avoid markup

    def _set_current_page(self, current_page, warning_text=None):
        """Hide all the pages but 'current_page'."""
        if hasattr(current_page, 'header'):
            self._set_header(current_page.header)

        if hasattr(current_page, 'help_text'):
            self.help_label.set_markup(current_page.help_text)

        if warning_text is not None:
            self._set_warning_message(self.warning_label, warning_text)
        else:
            self.warning_label.hide()

        for page in self.pages:
            if page is current_page:
                page.show()
            else:
                page.hide()

        if current_page.default_widget is not None:
            current_page.default_widget.grab_default()

    def _generate_captcha(self):
        """Ask for a new captcha; update the ui to reflect the fact."""
        logger.info('Calling generate_captcha with filename path at %r',
                    self._captcha_filename)
        self.backend.generate_captcha(self.app_name, self._captcha_filename,
                                      reply_handler=NO_OP, error_handler=NO_OP)
        self._set_captcha_loading()

    def _set_captcha_loading(self):
        """Present a spinner to the user while the captcha is downloaded."""
        self.captcha_image.hide()
        self._add_spinner_to_container(self.captcha_loading)
        white = gtk.gdk.Color('white')
        self.captcha_loading.modify_bg(gtk.STATE_NORMAL, white)
        self.captcha_loading.show_all()
        self.join_ok_button.set_sensitive(False)

    def _set_captcha_image(self):
        """Present a captcha image to the user to be resolved."""
        self.captcha_loading.hide()
        self.join_ok_button.set_sensitive(True)
        self.captcha_image.set_from_file(self._captcha_filename)
        self.captcha_image.show()

    def _build_enter_details_page(self):
        """Build the enter details page."""
        d = {'app_name': self.app_label}
        self.enter_details_vbox.header = self.JOIN_HEADER_LABEL % d
        self.enter_details_vbox.help_text = self.help_text
        self.enter_details_vbox.default_widget = self.join_ok_button
        self.join_ok_button.set_flags(gtk.CAN_DEFAULT)

        self.enter_details_vbox.pack_start(self.name_entry, expand=False)
        self.enter_details_vbox.reorder_child(self.name_entry, 0)
        entry = self.captcha_solution_entry
        self.captcha_solution_vbox.pack_start(entry, expand=False)
        self.captcha_solution_vbox.reorder_child(entry, 0)
        msg = self.CAPTCHA_RELOAD_TOOLTIP
        self.captcha_reload_button.set_tooltip_text(msg)

        self.emails_hbox.pack_start(self.email1_entry, expand=False)
        self.emails_hbox.pack_start(self.email2_entry, expand=False)

        self.passwords_hbox.pack_start(self.password1_entry, expand=False)
        self.passwords_hbox.pack_start(self.password2_entry, expand=False)
        help_msg = '<small>%s</small>' % self.PASSWORD_HELP
        self.password_help_label.set_markup(help_msg)

        if not os.path.exists(self._captcha_filename):
            self._generate_captcha()
        else:
            self._set_captcha_image()

        msg = self.YES_TO_UPDATES % {'app_name': self.app_name}
        self.yes_to_updates_checkbutton.set_label(msg)
        if self.tc_uri:
            msg = self.YES_TO_TC % {'app_name': self.app_name}
            self.yes_to_tc_checkbutton.set_label(msg)
            self.tc_button.set_label(self.TC)
        else:
            self.tc_hbox.hide_all()
        self.login_button.set_label(self.LOGIN_BUTTON_LABEL)

        for entry in (self.name_entry, self.email1_entry, self.email2_entry,
                      self.password1_entry, self.password2_entry,
                      self.captcha_solution_entry):
            entry.connect('activate', lambda w: self.join_ok_button.clicked())

        return self.enter_details_vbox

    def _build_tc_page(self):
        """Build the Terms & Conditions page."""
        self.tc_browser_vbox.help_text = ''
        self.tc_browser_vbox.default_widget = self.tc_back_button
        self.tc_browser_vbox.default_widget.set_flags(gtk.CAN_DEFAULT)
        return self.tc_browser_vbox

    def _build_processing_page(self):
        """Build the processing page with a spinner."""
        self.processing_vbox.default_widget = None
        self._add_spinner_to_container(self.processing_vbox,
                                       legend=self.ONE_MOMENT_PLEASE)
        return self.processing_vbox

    def _build_verify_email_page(self):
        """Build the verify email page."""
        self.verify_email_vbox.default_widget = self.verify_token_button
        self.verify_email_vbox.default_widget.set_flags(gtk.CAN_DEFAULT)

        cb = lambda w: self.verify_token_button.clicked()
        self.email_token_entry.connect('activate', cb)
        self.verify_email_vbox.pack_start(self.email_token_entry, expand=False)
        self.verify_email_vbox.reorder_child(self.email_token_entry, 0)

        return self.verify_email_vbox

    def _build_success_page(self):
        """Build the success page."""
        self.success_vbox.default_widget = self.success_close_button
        self.success_vbox.default_widget.set_flags(gtk.CAN_DEFAULT)

        self.success_label.set_markup('<span size="x-large">%s</span>' %
                                      self.SUCCESS)
        return self.success_vbox

    def _build_login_page(self):
        """Build the login page."""
        d = {'app_name': self.app_label}
        self.login_vbox.header = self.LOGIN_HEADER_LABEL % d
        self.login_vbox.help_text = self.CONNECT_HELP_LABEL % d
        self.login_vbox.default_widget = self.login_ok_button
        self.login_vbox.default_widget.set_flags(gtk.CAN_DEFAULT)

        self.login_details_vbox.pack_start(self.login_email_entry)
        self.login_details_vbox.reorder_child(self.login_email_entry, 0)
        self.login_details_vbox.pack_start(self.login_password_entry)
        self.login_details_vbox.reorder_child(self.login_password_entry, 1)

        msg = self.FORGOTTEN_PASSWORD_BUTTON
        self.forgotten_password_button.set_label(msg)
        self.login_ok_button.grab_focus()

        for entry in (self.login_email_entry, self.login_password_entry):
            entry.connect('activate', lambda w: self.login_ok_button.clicked())

        return self.login_vbox

    def _build_request_password_token_page(self):
        """Build the login page."""
        self.request_password_token_vbox.header = self.RESET_PASSWORD
        text = self.REQUEST_PASSWORD_TOKEN_LABEL % {'app_name': self.app_label}
        self.request_password_token_vbox.help_text = text
        btn = self.request_password_token_ok_button
        btn.set_flags(gtk.CAN_DEFAULT)
        self.request_password_token_vbox.default_widget = btn

        entry = self.reset_email_entry
        self.request_password_token_details_vbox.pack_start(entry,
                                                            expand=False)
        cb = self.on_reset_email_entry_changed
        self.reset_email_entry.connect('changed', cb)
        self.request_password_token_ok_button.set_label(self.NEXT)
        self.request_password_token_ok_button.set_sensitive(False)

        cb = lambda w: self.request_password_token_ok_button.clicked()
        self.reset_email_entry.connect('activate', cb)

        return self.request_password_token_vbox

    def _build_set_new_password_page(self):
        """Build the login page."""
        self.set_new_password_vbox.header = self.RESET_PASSWORD
        self.set_new_password_vbox.help_text = self.SET_NEW_PASSWORD_LABEL
        btn = self.set_new_password_ok_button
        btn.set_flags(gtk.CAN_DEFAULT)
        self.set_new_password_vbox.default_widget = btn

        cb = lambda w: self.set_new_password_ok_button.clicked()
        for entry in (self.reset_code_entry,
                      self.reset_password1_entry,
                      self.reset_password2_entry):
            self.set_new_password_details_vbox.pack_start(entry, expand=False)
            entry.connect('activate', cb)

        cb = self.on_set_new_password_entries_changed
        self.reset_code_entry.connect('changed', cb)
        self.reset_password1_entry.connect('changed', cb)
        self.reset_password2_entry.connect('changed', cb)
        help_msg = '<small>%s</small>' % self.PASSWORD_HELP
        self.reset_password_help_label.set_markup(help_msg)

        self.set_new_password_ok_button.set_label(self.RESET_PASSWORD)
        self.set_new_password_ok_button.set_sensitive(False)

        return self.set_new_password_vbox

    def _validate_email(self, email1, email2=None):
        """Validate 'email1', return error message if not valid.

        If 'email2' is given, must match 'email1'.
        """
        if email2 is not None and email1 != email2:
            return self.EMAIL_MISMATCH

        if not email1:
            return self.FIELD_REQUIRED

        if '@' not in email1:
            return self.EMAIL_INVALID

    def _validate_password(self, password1, password2=None):
        """Validate 'password1', return error message if not valid.

        If 'password2' is given, must match 'email1'.
        """
        if password2 is not None and password1 != password2:
            return self.PASSWORD_MISMATCH

        if (len(password1) < 8 or
            re.search('[A-Z]', password1) is None or
            re.search('\d+', password1) is None):
            return self.PASSWORD_TOO_WEAK

    # GTK callbacks

    def run(self):
        """Run the application."""
        gtk.main()

    def connect(self, signal_name, handler, *args, **kwargs):
        """Connect 'signal_name' with 'handler'."""
        logger.debug('connect: signal %r, handler %r, args  %r, kwargs, %r',
                     signal_name, handler, args, kwargs)
        self.window.connect(signal_name, handler, *args, **kwargs)

    def on_close_clicked(self, *args, **kwargs):
        """Call self.close_callback if defined."""
        if os.path.exists(self._captcha_filename):
            os.remove(self._captcha_filename)

        # remove the signals from DBus
        remove = self.bus.remove_signal_receiver
        for (iface, signal) in self._signals_receivers.keys():
            method = self._signals_receivers.pop((iface, signal))
            logger.debug('Removing signal %r with method %r at iface %r.',
                         signal, method, iface)
            remove(method, signal_name=signal, dbus_interface=iface)

        # hide the main window
        if self.window is not None:
            self.window.hide()

        # process any pending events before emitting signals
        while gtk.events_pending():
            gtk.main_iteration()

        if len(args) > 0 and args[0] in self.cancels:
            self.window.emit(SIG_USER_CANCELATION, self.app_name)
        elif len(self._gtk_signal_log) > 0:
            signal = self._gtk_signal_log[-1][0]
            args = self._gtk_signal_log[-1][1:]
            self.window.emit(signal, *args)
        else:
            self.window.emit(SIG_USER_CANCELATION, self.app_name)

        # call user defined callback
        if self.close_callback is not None:
            logger.info('Calling custom close_callback %r with params %r, %r',
                        self.close_callback, args, kwargs)
            self.close_callback(*args, **kwargs)

    def on_sign_in_button_clicked(self, *args, **kwargs):
        """User wants to sign in, present the Login page."""
        self._set_current_page(self.login_vbox)

    def on_join_ok_button_clicked(self, *args, **kwargs):
        """Submit info for processing, present the processing vbox."""
        if not self.join_ok_button.is_sensitive():
            return

        self._clear_warnings()

        error = False

        name = self.name_entry.get_text()
        if not name:
            self.name_entry.set_warning(self.FIELD_REQUIRED)
            error = True

        # check email
        email1 = self.email1_entry.get_text()
        email2 = self.email2_entry.get_text()
        msg = self._validate_email(email1, email2)
        if msg is not None:
            self.email1_entry.set_warning(msg)
            self.email2_entry.set_warning(msg)
            error = True

        # check password
        password1 = self.password1_entry.get_text()
        password2 = self.password2_entry.get_text()
        msg = self._validate_password(password1, password2)
        if msg is not None:
            self.password1_entry.set_warning(msg)
            self.password2_entry.set_warning(msg)
            error = True

        # check T&C
        if not self.yes_to_tc_checkbutton.get_active():
            self._set_warning_message(self.tc_warning_label,
                                      self.TC_NOT_ACCEPTED)
            error = True

        captcha_solution = self.captcha_solution_entry.get_text()
        if not captcha_solution:
            self.captcha_solution_entry.set_warning(self.FIELD_REQUIRED)
            error = True

        if error:
            return

        self._set_current_page(self.processing_vbox)
        self.user_email = email1
        self.user_password = password1

        logger.info('Calling register_with_name with email %r, password, '
                    '<hidden> name %r, captcha_id %r and captcha_solution %r.',
                    email1, name, self._captcha_id, captcha_solution)
        f = self.backend.register_with_name
        f(self.app_name, email1, password1, name,
          self._captcha_id, captcha_solution,
          reply_handler=NO_OP, error_handler=NO_OP)

    def on_tc_button_clicked(self, *args, **kwargs):
        """T & C button was clicked, show the browser with them."""
        self._set_current_page(self.tc_browser_vbox)

    def on_tc_back_button_clicked(self, *args, **kwargs):
        """T & C 'back' button was clicked, return to the previous page."""
        self._set_current_page(self.enter_details_vbox)

    def on_verify_token_button_clicked(self, *args, **kwargs):
        """The user entered the email token, let's verify!"""
        if not self.verify_token_button.is_sensitive():
            return

        self._clear_warnings()

        email_token = self.email_token_entry.get_text()
        if not email_token:
            self.email_token_entry.set_warning(self.FIELD_REQUIRED)
            return

        email = self.user_email
        password = self.user_password
        f = self.backend.validate_email
        logger.info('Calling validate_email with email %r, password <hidden>' \
                    ', app_name %r and email_token %r.', email, self.app_name,
                    email_token)
        f(self.app_name, email, password, email_token,
          reply_handler=NO_OP, error_handler=NO_OP)

        self._set_current_page(self.processing_vbox)

    def on_login_connect_button_clicked(self, *args, **kwargs):
        """User wants to connect!"""
        if not self.login_ok_button.is_sensitive():
            return

        self._clear_warnings()

        error = False

        email = self.login_email_entry.get_text()
        msg = self._validate_email(email)
        if msg is not None:
            self.login_email_entry.set_warning(msg)
            error = True

        password = self.login_password_entry.get_text()
        if not password:
            self.login_password_entry.set_warning(self.FIELD_REQUIRED)
            error = True

        if error:
            return

        f = self.backend.login
        f(self.app_name, email, password,
          reply_handler=NO_OP, error_handler=NO_OP)

        self._set_current_page(self.processing_vbox)
        self.user_email = email
        self.user_password = password

    def on_login_back_button_clicked(self, *args, **kwargs):
        """User wants to go to the previous page."""
        self._set_current_page(self.enter_details_vbox)

    def on_forgotten_password_button_clicked(self, *args, **kwargs):
        """User wants to reset the password."""
        self._set_current_page(self.request_password_token_vbox)

    def on_request_password_token_ok_button_clicked(self, *args, **kwargs):
        """User entered the email address to reset the password."""
        if not self.request_password_token_ok_button.is_sensitive():
            return

        self._clear_warnings()

        email = self.reset_email_entry.get_text()
        msg = self._validate_email(email)
        if msg is not None:
            self.reset_email_entry.set_warning(msg)
            return

        logger.info('Calling request_password_reset_token with %r.', email)
        f = self.backend.request_password_reset_token
        f(self.app_name, email, reply_handler=NO_OP, error_handler=NO_OP)

        self._set_current_page(self.processing_vbox)

    def on_request_password_token_back_button_clicked(self, *args, **kwargs):
        """User wants to go to the previous page."""
        self._set_current_page(self.login_vbox)

    def on_reset_email_entry_changed(self, widget, *args, **kwargs):
        """User is changing the 'widget' entry in the reset email page."""
        sensitive = self._non_empty_input(widget)
        self.request_password_token_ok_button.set_sensitive(sensitive)

    def on_set_new_password_entries_changed(self, *args, **kwargs):
        """User is changing the 'widget' entry in the reset password page."""
        sensitive = True
        for entry in (self.reset_code_entry,
                      self.reset_password1_entry,
                      self.reset_password2_entry):
            sensitive &= self._non_empty_input(entry)
        self.set_new_password_ok_button.set_sensitive(sensitive)

    def on_set_new_password_ok_button_clicked(self, *args, **kwargs):
        """User entered reset code and new passwords."""
        if not self.set_new_password_ok_button.is_sensitive():
            return

        self._clear_warnings()

        error = False

        token = self.reset_code_entry.get_text()
        if not token:
            self.reset_code_entry.set_warning(self.FIELD_REQUIRED)
            error = True

        password1 = self.reset_password1_entry.get_text()
        password2 = self.reset_password2_entry.get_text()
        msg = self._validate_password(password1, password2)
        if msg is not None:
            self.reset_password1_entry.set_warning(msg)
            self.reset_password2_entry.set_warning(msg)
            error = True

        if error:
            return

        email = self.reset_email_entry.get_text()
        logger.info('Calling set_new_password with email %r, token %r and ' \
                    'new password: <hidden>.', email, token)
        f = self.backend.set_new_password
        f(self.app_name, email, token, password1,
          reply_handler=NO_OP, error_handler=NO_OP)

        self._set_current_page(self.processing_vbox)

    def on_tc_browser_vbox_show(self, *args, **kwargs):
        """The T&C page is being shown."""
        browser = webkit.WebView()
        settings = browser.get_settings()
        settings.set_property("enable-plugins", False)
        settings.set_property("enable-default-context-menu", False)
        browser.open(self.tc_uri)
        browser.show()
        self.tc_browser_window.add(browser)

    def on_tc_browser_vbox_hide(self, *args, **kwargs):
        """The T&C page is no longer being shown."""
        children = self.tc_browser_window.get_children()
        if len(children) > 0:
            browser = children[0]
            self.tc_browser_window.remove(browser)
            browser.destroy()
            del browser

    def on_captcha_reload_button_clicked(self, *args, **kwargs):
        """User clicked the reload captcha button."""
        self._generate_captcha()

    # backend callbacks

    def _build_general_error_message(self, errordict):
        """Concatenate __all__ and message from the errordict."""
        result = None
        msg1 = errordict.get('__all__')
        msg2 = errordict.get('message')
        if msg1 is not None and msg2 is not None:
            result = '\n'.join((msg1, msg2))
        else:
            result = msg1 if msg1 is not None else msg2
        return result

    @log_call
    def on_captcha_generated(self, app_name, captcha_id, *args, **kwargs):
        """Captcha image has been generated and is available to be shown."""
        assert captcha_id is not None
        self._captcha_id = captcha_id
        self._set_captcha_image()

    @log_call
    def on_captcha_generation_error(self, app_name, error, *args, **kwargs):
        """Captcha image generation failed."""

    @log_call
    def on_user_registered(self, app_name, email, *args, **kwargs):
        """Registration can go on, user needs to verify email."""
        help_text = self.VERIFY_EMAIL_LABEL % {'app_name': self.app_name,
                                               'email': email}
        self.verify_email_vbox.help_text = help_text
        self._set_current_page(self.verify_email_vbox)

    @log_call
    def on_user_registration_error(self, app_name, error, *args, **kwargs):
        """Captcha image generation failed."""
        msg = error.get('email')
        if msg is not None:
            self.email1_entry.set_warning(msg)
            self.email2_entry.set_warning(msg)

        msg = error.get('password')
        if msg is not None:
            self.password1_entry.set_warning(msg)
            self.password2_entry.set_warning(msg)

        msg = self._build_general_error_message(error)
        self._set_current_page(self.enter_details_vbox, warning_text=msg)

        self._gtk_signal_log.append((SIG_REGISTRATION_FAILED, self.app_name,
                                    msg))

    @log_call
    def on_email_validated(self, app_name, email, *args, **kwargs):
        """User email was successfully verified."""
        self._set_current_page(self.success_vbox)
        self._gtk_signal_log.append((SIG_REGISTRATION_SUCCEEDED, self.app_name,
                                    email))

    @log_call
    def on_email_validation_error(self, app_name, error, *args, **kwargs):
        """User email validation failed."""
        msg = error.get('email_token')
        if msg is not None:
            self.email_token_entry.set_warning(msg)

        msg = self._build_general_error_message(error)
        self._set_current_page(self.verify_email_vbox, warning_text=msg)

        self._gtk_signal_log.append((SIG_REGISTRATION_FAILED, self.app_name,
                                     msg))

    @log_call
    def on_logged_in(self, app_name, email, *args, **kwargs):
        """User was successfully logged in."""
        self._set_current_page(self.success_vbox)
        self._gtk_signal_log.append((SIG_LOGIN_SUCCEEDED, self.app_name,
                                    email))

    @log_call
    def on_login_error(self, app_name, error, *args, **kwargs):
        """User was not successfully logged in."""
        msg = self._build_general_error_message(error)
        self._set_current_page(self.login_vbox, warning_text=msg)
        self._gtk_signal_log.append((SIG_LOGIN_FAILED, self.app_name,
                                     msg))

    @log_call
    def on_user_not_validated(self, app_name, email, *args, **kwargs):
        """User was not validated."""
        self.on_user_registered(app_name, email)

    @log_call
    def on_password_reset_token_sent(self, app_name, email, *args, **kwargs):
        """Password reset token was successfully sent."""
        msg = self.SET_NEW_PASSWORD_LABEL % {'email': email}
        self.set_new_password_vbox.help_text = msg
        self._set_current_page(self.set_new_password_vbox)

    @log_call
    def on_password_reset_error(self, app_name, error, *args, **kwargs):
        """Password reset failed."""
        msg = self._build_general_error_message(error)
        self._set_current_page(self.login_vbox, warning_text=msg)

    @log_call
    def on_password_changed(self, app_name, email, *args, **kwargs):
        """Password was successfully changed."""
        self._set_current_page(self.login_vbox,
                               warning_text=self.PASSWORD_CHANGED)

    @log_call
    def on_password_change_error(self, app_name, error, *args, **kwargs):
        """Password reset failed."""
        msg = self._build_general_error_message(error)
        self._set_current_page(self.request_password_token_vbox,
                               warning_text=msg)
