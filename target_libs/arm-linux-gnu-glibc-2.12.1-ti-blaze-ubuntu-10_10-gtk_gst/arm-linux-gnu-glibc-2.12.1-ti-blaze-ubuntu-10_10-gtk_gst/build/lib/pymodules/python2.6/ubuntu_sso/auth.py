# ubuntu_sso.auth - Client authorization module
#
# Author: Stuart Langridge <stuart.langridge@canonical.com>
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
# PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
"""This is an obsolete module.

Do not use it, use the DBus API from main instead (SSOCredentials).
"""


class NoAccessToken(Exception):
    """No access token available."""
