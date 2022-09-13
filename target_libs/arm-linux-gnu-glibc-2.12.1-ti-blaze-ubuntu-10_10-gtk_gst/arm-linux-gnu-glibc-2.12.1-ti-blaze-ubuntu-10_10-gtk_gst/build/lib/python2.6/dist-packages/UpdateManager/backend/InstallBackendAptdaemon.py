#!/usr/bin/env python
# -*- coding: utf-8 -*-
# (c) 2005-2009 Canonical, GPL

from aptdaemon import client, errors
from aptdaemon.defer import inline_callbacks
from aptdaemon.gtkwidgets import AptProgressDialog

from UpdateManager.backend import InstallBackend
import apt_pkg

class InstallBackendAptdaemon(InstallBackend):

    """Makes use of aptdaemon to refresh the cache and to install updates."""

    def __init__(self, window_main):
        InstallBackend.__init__(self, window_main)
        self.client = client.AptClient()

    @inline_callbacks
    def update(self):
        """Refresh the package list"""
        try:
            apt_pkg.PkgSystemUnLock()
        except SystemError:
            pass
        try:
            trans = yield self.client.update_cache(defer=True)
            self._run_in_dialog(trans, self.UPDATE)
        except errors.NotAuthorizedError:
            self.emit("action-done", self.UPDATE, False)
        except:
            self.emit("action-done", self.UPDATE, True)
            raise

    @inline_callbacks
    def commit(self, pkgs_install, pkgs_upgrade, close_on_done):
        """Commit a list of package adds and removes"""
        try:
            apt_pkg.PkgSystemUnLock()
        except SystemError:
            pass
        try:
            trans = yield self.client.commit_packages(pkgs_install, [], [],
                                                      [], pkgs_upgrade,
                                                      defer=True)
            self._run_in_dialog(trans, self.INSTALL)
        except errors.NotAuthorizedError:
            self.emit("action-done", self.INSTALL, False)
        except:
            self.emit("action-done", self.INSTALL, True)
            raise

    def _run_in_dialog(self, trans, action):
        dia = AptProgressDialog(trans, parent=self.window_main)
        dia.set_icon_name("update-manager")
        dia.connect("finished", self._on_finished, action)
        dia.run()

    def _on_finished(self, dialog, action):
        dialog.hide()
        self.emit("action-done", action, True)
