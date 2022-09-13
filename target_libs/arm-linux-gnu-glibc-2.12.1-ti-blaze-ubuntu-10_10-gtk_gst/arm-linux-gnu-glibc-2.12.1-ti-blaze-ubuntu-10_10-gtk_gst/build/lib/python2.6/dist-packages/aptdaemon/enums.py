#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""enums - Enumerates for apt daemon dbus messages"""
# Copyright (C) 2008-2009 Sebastian Heinlein <devel@glatzor.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

__author__  = "Sebastian Heinlein <devel@glatzor.de>"

import gettext
def _(msg):
   return gettext.dgettext("aptdaemon", msg)

# Index of package groups in the depends and packages property
(PKGS_INSTALL,
 PKGS_REINSTALL,
 PKGS_REMOVE,
 PKGS_PURGE,
 PKGS_UPGRADE,
 PKGS_DOWNGRADE,
 PKGS_KEEP) = range(7)

# Finish states
EXIT_SUCCESS = "exit-success"
EXIT_CANCELLED = "exit-cancelled"
EXIT_FAILED = "exit-failed"
EXIT_PREVIOUS_FAILED = "exit-previous-failed"
EXIT_UNFINISHED = "exit-unfinished"

# Error codes
ERROR_PACKAGE_DOWNLOAD_FAILED = "error-package-download-failed"
ERROR_REPO_DOWNLOAD_FAILED = "error-repo-download-failed"
ERROR_DEP_RESOLUTION_FAILED = "error-dep-resolution-failed"
ERROR_KEY_NOT_INSTALLED = "error-key-not-installed"
ERROR_KEY_NOT_REMOVED = "error-key-not-removed"
ERROR_NO_LOCK = "error-no-lock"
ERROR_NO_CACHE = "error-no-cache"
ERROR_NO_PACKAGE = "error-no-package"
ERROR_PACKAGE_UPTODATE = "error-package-uptodate"
ERROR_PACKAGE_ALREADY_INSTALLED = "error-package-already-installed"
ERROR_PACKAGE_NOT_INSTALLED = "error-package-not-installed"
ERROR_NOT_REMOVE_ESSENTIAL_PACKAGE = "error-not-remove-essential"
ERROR_DAEMON_DIED = "error-daemon-died"
ERROR_PACKAGE_MANAGER_FAILED = "error-package-manager-failed"
ERROR_CACHE_BROKEN = "error-cache-broken"
ERROR_PACKAGE_UNAUTHENTICATED = "error-package-unauthenticated"
ERROR_INCOMPLETE_INSTALL = "error-incomplete-install"
ERROR_UNKNOWN = "error-unknown"

# Status codes
STATUS_SETTING_UP = "status-setting-up"
STATUS_WAITING = "status-waiting"
STATUS_WAITING_MEDIUM = "status-waiting-medium"
STATUS_WAITING_CONFIG_FILE_PROMPT = "status-waiting-config-file-prompt"
STATUS_WAITING_LOCK = "status-waiting-lock"
STATUS_RUNNING = "status-running"
STATUS_LOADING_CACHE = "status-loading-cache"
STATUS_DOWNLOADING = "status-downloading"
STATUS_COMMITTING = "status-committing"
STATUS_CLEANING_UP = "status-cleaning-up"
STATUS_RESOLVING_DEP = "status-resolving-dep"
STATUS_FINISHED = "status-finished"
STATUS_CANCELLING = "status-cancelling"

ROLE_UNSET = "role-unset"
ROLE_INSTALL_PACKAGES = "role-install-packages"
ROLE_INSTALL_FILE = "role-install-file"
ROLE_UPGRADE_PACKAGES = "role-upgrade-packages"
ROLE_UPGRADE_SYSTEM = "role-upgrade-system"
ROLE_UPDATE_CACHE = "role-update-cache"
ROLE_REMOVE_PACKAGES = "role-remove-packages"
ROLE_COMMIT_PACKAGES = "role-commit-packages"
ROLE_ADD_VENDOR_KEY_FILE = "role-add-vendor-key-file"
ROLE_ADD_VENDOR_KEY_FROM_KEYSERVER = "role-add-vendor-key-from-keyserver"
ROLE_REMOVE_VENDOR_KEY = "role-remove-vendor-key"
ROLE_FIX_INCOMPLETE_INSTALL = "role-fix-incomplete-install"
ROLE_FIX_BROKEN_DEPENDS = "role-fix-broken-depends"
ROLE_ADD_REPOSITORY = "role-add-repository"
ROLE_ENABLE_DISTRO_COMP = "role-enable-distro-comp"

ICONS_STATUS = {
    STATUS_CANCELLING:'aptdaemon-cleanup',
    STATUS_CLEANING_UP:'aptdaemon-cleanup',
    STATUS_RESOLVING_DEP:'aptdaemon-resolve',
    STATUS_COMMITTING:'aptdaemon-working',
    STATUS_DOWNLOADING:'aptdaemon-download',
    STATUS_FINISHED:'aptdaemon-cleanup',
    STATUS_LOADING_CACHE:'aptdaemon-update-cache',
    STATUS_RUNNING:'aptdaemon-working',
    STATUS_SETTING_UP:'aptdaemon-working',
    STATUS_WAITING:'aptdaemon-wait',
    STATUS_WAITING_LOCK:'aptdaemon-wait',
    STATUS_WAITING_MEDIUM:'aptdaemon-wait',
    STATUS_WAITING_CONFIG_FILE_PROMPT:'aptdaemon-wait',
    }

def get_status_icon_name_from_enum(enum):
   if ICONS_STATUS.has_key(enum):
       return ICONS_STATUS[enum]
   else:
       return "aptdaemon-working"

ICONS_ROLE = {
    ROLE_INSTALL_FILE:'aptdaemon-add',
    ROLE_INSTALL_PACKAGES:'aptdaemon-add',
    ROLE_UPDATE_CACHE:'aptdaemon-update-cache',
    ROLE_REMOVE_PACKAGES:'aptdaemon-delete',
    ROLE_UPGRADE_PACKAGES:'aptdaemon-upgrade',
    ROLE_UPGRADE_SYSTEM:'system-software-update',
    }

def get_role_icon_name_from_enum(enum):
   if ICONS_ROLE.has_key(enum):
       return ICONS_ROLE[enum]
   else:
       return "aptdaemon-working"

ANIMATIONS_STATUS = {
    STATUS_CANCELLING:'aptdaemon-action-cleaning-up',
    STATUS_CLEANING_UP:'aptdaemon-action-cleaning-up',
    STATUS_RESOLVING_DEP:'aptdaemon-action-resolving',
    STATUS_DOWNLOADING:'aptdaemon-action-downloading',
    STATUS_LOADING_CACHE:'aptdaemon-action-updating-cache',
    STATUS_WAITING:'aptdaemon-action-waiting',
    STATUS_WAITING_LOCK:'aptdaemon-action-waiting',
    STATUS_WAITING_MEDIUM:'aptdaemon-action-waiting',
    STATUS_WAITING_CONFIG_FILE_PROMPT:'aptdaemon-action-waiting',
    }


def get_status_animation_name_from_enum(enum):
   if ANIMATIONS_STATUS.has_key(enum):
       return ANIMATIONS_STATUS[enum]
   else:
       return None

PAST_ROLE = {
    ROLE_INSTALL_FILE : _("Installed file"),
    ROLE_INSTALL_PACKAGES : _("Installed packages"),
    ROLE_ADD_VENDOR_KEY_FILE: _("Added key from file"),
    ROLE_UPDATE_CACHE : _("Updated cache"),
    ROLE_REMOVE_VENDOR_KEY : _("Removed trusted key"),
    ROLE_REMOVE_PACKAGES : _("Removed packages"),
    ROLE_UPGRADE_PACKAGES : _("Updated packages"),
    ROLE_UPGRADE_SYSTEM : _("Upgraded system"),
    ROLE_COMMIT_PACKAGES : _("Applied changes"),
    ROLE_FIX_INCOMPLETE_INSTALL : _("Repaired incomplete installation"),
    ROLE_FIX_BROKEN_DEPENDS: _("Repaired broken dependencies"),
    ROLE_ADD_REPOSITORY: _("Added software source"),
    ROLE_ENABLE_DISTRO_COMP: _("Enabled component of the distribution"),
    ROLE_UNSET: ""
    }

def get_role_localised_past_from_enum(enum):
   if PAST_ROLE.has_key(enum):
       return PAST_ROLE[enum]
   else:
       return None

STRING_EXIT = {
    EXIT_SUCCESS : _("Successful"),
    EXIT_CANCELLED : _("Canceled"),
    EXIT_FAILED : _("Failed")
    }

def get_exit_string_from_enum(enum):
   if STRING_EXIT.has_key(enum):
       return STRING_EXIT[enum]
   else:
       return None

PRESENT_ROLE = {
    ROLE_INSTALL_FILE : _("Installing file"),
    ROLE_INSTALL_PACKAGES : _("Installing packages"),
    ROLE_ADD_VENDOR_KEY_FILE : _("Adding key from file"),
    ROLE_UPDATE_CACHE : _("Updating cache"),
    ROLE_REMOVE_VENDOR_KEY : _("Removing trusted key"),
    ROLE_REMOVE_PACKAGES : _("Removing packages"),
    ROLE_UPGRADE_PACKAGES : _("Updating packages"),
    ROLE_UPGRADE_SYSTEM : _("Upgrading system"),
    ROLE_COMMIT_PACKAGES : _("Applying changes"),
    ROLE_FIX_INCOMPLETE_INSTALL : _("Repairing incomplete installation"),
    ROLE_FIX_BROKEN_DEPENDS: _("Repairing broken deps"),
    ROLE_ADD_REPOSITORY: _("Adding software source"),
    ROLE_ENABLE_DISTRO_COMP: _("Enabling component of the distribution"),
    ROLE_UNSET : ""
    }

def get_role_localised_present_from_enum(enum):
   if PRESENT_ROLE.has_key(enum):
       return PRESENT_ROLE[enum]
   else:
       return None

ERROR_ROLE = {
    ROLE_INSTALL_FILE: _("Installation of the package file failed"),
    ROLE_INSTALL_PACKAGES: _("Installation of software failed"),
    ROLE_ADD_VENDOR_KEY_FILE: _("Adding the key to the list of trused "
                                "software vendors failed"),
    ROLE_UPDATE_CACHE: _("Refreshing the software list failed"),
    ROLE_REMOVE_VENDOR_KEY: _("Removing the vendor from the list of trusted "
                              "ones failed"),
    ROLE_REMOVE_PACKAGES: _("Removing software failed"),
    ROLE_UPGRADE_PACKAGES: _("Updating software failed"),
    ROLE_UPGRADE_SYSTEM: _("Upgrading the system failed"),
    ROLE_COMMIT_PACKAGES: _("Applying software changes failed"),
    ROLE_FIX_INCOMPLETE_INSTALL: _("Repairing incomplete installation failed"),
    ROLE_FIX_BROKEN_DEPENDS: _("Repairing broken dependencies failed"),
    ROLE_ADD_REPOSITORY: _("Adding software source failed"),
    ROLE_ENABLE_DISTRO_COMP: _("Enabling component of the distribution failed"),
    ROLE_UNSET: ""
    }

def get_role_error_from_enum(enum):
   if ERROR_ROLE.has_key(enum):
       return ERROR_ROLE[enum]
   else:
       return None

DESCS_ERROR = {
    ERROR_PACKAGE_DOWNLOAD_FAILED : _("Check your Internet connection."),
    ERROR_REPO_DOWNLOAD_FAILED : _("Check your Internet connection."),
    ERROR_CACHE_BROKEN : _("Check if you are using third party "
                           "repositories. If so disable them, since "
                           "they are a common source of problems.\n"
                           "Furthermore run the following command in a "
                           "Terminal: apt-get install -f"),
    ERROR_KEY_NOT_INSTALLED : _("The selected file may not be a GPG key file "
                                "or it might be corrupt."),
    ERROR_KEY_NOT_REMOVED : _("The selected key couldn't be removed. "
                              "Check that you provided a valid fingerprint."),
    ERROR_NO_LOCK : _("Check if you are currently running another "
                      "software management tool, e.g. Synaptic or aptitude. "
                      "Only one tool is allowed to make changes at a time."),
    ERROR_NO_CACHE : _("This is a serious problem. Try again later. If this "
                       "problem appears again, please report an error to the "
                       "developers."),
    ERROR_NO_PACKAGE : _("Check the spelling of the package name, and "
                         "that the appropriate repository is enabled."),
    ERROR_PACKAGE_UPTODATE : _("There isn't any need for an update."),
    ERROR_PACKAGE_ALREADY_INSTALLED : _("There isn't any need for an "
                                        "installation"),
    ERROR_PACKAGE_NOT_INSTALLED : _("There isn't any need for a removal."),
    ERROR_NOT_REMOVE_ESSENTIAL_PACKAGE : _("You requested to remove a package "
                                           "which is an essential part of your "
                                           "system."),
    ERROR_DAEMON_DIED : _("The connection to the daemon was lost. Most likely "
                          "the background daemon crashed."),
    ERROR_PACKAGE_MANAGER_FAILED: _("The installation or removal of a software "
                                    "package failed."),
    ERROR_UNKNOWN : _("There seems to be a programming error in aptdaemon, "
                      "the software that allows you to install/remove "
                      "software and to perform other package management "
                      "related tasks. Please report this error at "
                      "http://launchpad.net/aptdaemon/+filebug and retry."),
    ERROR_DEP_RESOLUTION_FAILED: _("This error could be caused by required "
                                   "additional software packages which are "
                                   "missing or not installable. Furthermore "
                                   "there could be a conflict between software "
                                   "packages which are not allowed to be "
                                   "installed at the same time."),
    ERROR_PACKAGE_UNAUTHENTICATED: _("The action would require the "
                                     "installation of packages from not "
                                     "authenticated sources."),
    ERROR_INCOMPLETE_INSTALL: _("The installation could have failed because "
                                "of an error in the corresponding software "
                                "package or it was cancelled in an unfriendly "
                                "way. "
                                "You have to repair this before you can "
                                "install or remove any further software."),
    }

def get_error_description_from_enum(enum):
   if DESCS_ERROR.has_key(enum):
       return DESCS_ERROR[enum]
   else:
       return None

STRINGS_ERROR = {
    ERROR_PACKAGE_DOWNLOAD_FAILED : _("Failed to download package files"),
    ERROR_REPO_DOWNLOAD_FAILED : _("Failed to download repository information"),
    ERROR_DEP_RESOLUTION_FAILED : _("Package dependencies cannot be resolved"),
    ERROR_CACHE_BROKEN : _("The package system is broken"),
    ERROR_KEY_NOT_INSTALLED : _("Key was not installed"),
    ERROR_KEY_NOT_REMOVED: _("Key was not removed"),
    ERROR_NO_LOCK : _("Failed to lock the package manager"),
    ERROR_NO_CACHE : _("Failed to load the package list"),
    ERROR_NO_PACKAGE : _("Package does not exist"),
    ERROR_PACKAGE_UPTODATE : _("Package is already up-to-date"),
    ERROR_PACKAGE_ALREADY_INSTALLED : _("Package is already installed"),
    ERROR_PACKAGE_NOT_INSTALLED : _("Package isn't installed"),
    ERROR_NOT_REMOVE_ESSENTIAL_PACKAGE : _("Failed to remove essential "
                                           "system package"),
    ERROR_DAEMON_DIED : _("Task cannot be monitored or controlled"),
    ERROR_PACKAGE_MANAGER_FAILED: _("Package operation failed"),
    ERROR_PACKAGE_UNAUTHENTICATED: _("Requires installation of untrusted "
                                     "packages"),
    ERROR_INCOMPLETE_INSTALL: _("Previous installation hasn't been completed"),
    ERROR_UNKNOWN : _("An unhandlable error occured")
    }

def get_error_string_from_enum(enum):
   if STRINGS_ERROR.has_key(enum):
       return STRINGS_ERROR[enum]
   else:
       return None

STRINGS_STATUS = {
    STATUS_SETTING_UP : _("Waiting for service to start"),
    STATUS_WAITING : _("Waiting"),
    STATUS_WAITING_MEDIUM : _("Waiting for required medium"),
    STATUS_WAITING_LOCK : _("Waiting for other software managers to quit"),
    STATUS_WAITING_CONFIG_FILE_PROMPT : _("Waiting for configuration file "
                                          "prompt"),
    STATUS_RUNNING : _("Running task"),
    STATUS_DOWNLOADING : _("Downloading"),
    STATUS_CLEANING_UP : _("Cleaning up"),
    STATUS_RESOLVING_DEP : _("Resolving dependencies"),
    STATUS_COMMITTING : _("Applying changes"),
    STATUS_FINISHED : _("Finished"),
    STATUS_CANCELLING : _("Cancelling"),
    STATUS_LOADING_CACHE : _("Loading software list"),
    }

def get_status_string_from_enum(enum):
   if STRINGS_STATUS.has_key(enum):
       return STRINGS_STATUS[enum]
   else:
       return None

DOWNLOAD_DONE = "download-done"
DOWNLOAD_AUTH_ERROR = "download-auth-error"
DOWNLOAD_ERROR = "download-error"
DOWNLOAD_FETCHING = "download-fetching"
DOWNLOAD_IDLE = "download-idle"
DOWNLOAD_NETWORK_ERROR = "download-network-error"

STRINGS_DOWNLOAD = {
    DOWNLOAD_DONE: _("Done"),
    DOWNLOAD_AUTH_ERROR: _("Authentication failed"),
    DOWNLOAD_ERROR: _("Failed"),
    DOWNLOAD_FETCHING: _("Fetching"),
    DOWNLOAD_IDLE: _("Idle"),
    DOWNLOAD_NETWORK_ERROR: _("Network isn't available"),
    }

def get_download_status_from_enum(enum):
    try:
       return STRINGS_DOWNLOAD[enum]
    except KeyError:
       return None


# vim:ts=4:sw=4:et
