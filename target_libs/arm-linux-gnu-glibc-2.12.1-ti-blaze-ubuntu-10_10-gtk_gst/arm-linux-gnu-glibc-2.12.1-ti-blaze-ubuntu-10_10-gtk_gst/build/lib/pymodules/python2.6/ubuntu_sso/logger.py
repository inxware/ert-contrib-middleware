# ubuntu_sso.logger - logging miscellany
#
# Author: Stuart Langridge <stuart.langridge@canonical.com>
# Author: Natalia B. Bidart <natalia.bidart@canonical.com>
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

"""Miscellaneous logging functions."""

import logging
import os
import sys

import xdg.BaseDirectory

from logging.handlers import RotatingFileHandler

LOGFOLDER = os.path.join(xdg.BaseDirectory.xdg_cache_home, 'sso')
# create log folder if it doesn't exists
if not os.path.exists(LOGFOLDER):
    os.makedirs(LOGFOLDER)

LOGFILENAME = os.path.join(LOGFOLDER, 'oauth-login.log')
FMT = "%(asctime)s:%(msecs)s - %(name)s - %(levelname)s - %(message)s"

if os.environ.get('DEBUG'):
    LOG_LEVEL = logging.DEBUG
else:
    # Only log this level and above
    LOG_LEVEL = logging.INFO


def setup_logging(log_domain):
    """Create basic logger to set filename"""
    root_formatter = logging.Formatter(fmt=FMT)
    root_handler = RotatingFileHandler(LOGFILENAME, maxBytes=1048576,
                                       backupCount=5)
    root_handler.setLevel(LOG_LEVEL)
    root_handler.setFormatter(root_formatter)

    logger = logging.getLogger(log_domain)
    logger.propagate = False
    logger.setLevel(LOG_LEVEL)
    logger.addHandler(root_handler)
    if os.environ.get('DEBUG'):
        debug_handler = logging.StreamHandler(sys.stderr)
        logger.addHandler(debug_handler)

    return logger
