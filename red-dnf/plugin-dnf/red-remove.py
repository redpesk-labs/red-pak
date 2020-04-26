###########################################################################
# Copyright 2020 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

from __future__ import absolute_import
from __future__ import unicode_literals
from dnfpluginscore import _, logger
from dnf.cli.option_parser import OptionParser
from configparser import ConfigParser, NoOptionError, NoSectionError

import os
import warnings
import logging
logger = logging.getLogger('dnf')

import dnf
import dnf.logging
import reddnf

@dnf.plugin.register_command
class RedRemoveCommand(dnf.cli.Command):

    aliases = ['red-remove','rrm']
    summary = "Remove an installed rpm package in from redpath terminal redpah leaf"
    _dnfcmd = None

    def __init__(self, cli):
        super(RedRemoveCommand, self).__init__(cli)
        self.opts = None
        self.parser = None
        RedRemoveCommand._dnfcmd =  dnf.cli.commands.remove.RemoveCommand(cli)
        
    @staticmethod
    def set_argparser(parser):
        parser.add_argument("--redpath", help="container hierarchical rootfs path ex: /var/reddnf/platform/profile/project")
        RedRemoveCommand._dnfcmd.set_argparser(parser)

    # Never request any default dnf activation
    def configure(self):
        dnf.conf.installroot=self.opts.redpath
        demands = self.cli.demands
        demands.sack_activation = False
        demands.root_user = False
        demands.available_repos = False

        # needed for remove
        demands.resolving=True
        demands.allow_erasing=True

        # check we redpath is defined and exist
        if not self.opts.redpath:
            raise dnf.exceptions.Error("Syntax Error: redpak --redpath=/xx/../xyz subcommand (missing --redpath)")

        # Merge source default with main config environment
        self.redconf=reddnf.config.rednode_defaults(self.opts, None)

    def checkdir(self, label, dirpath, create=False):

        if create:
            try:
                os.mkdir(dirpath)
            except FileExistsError:
                pass
            except OSError:
                raise dnf.exceptions.Error("Directory {} does not exit path={}".format(label, dirpath))

        if not os.access(dirpath, os.W_OK):
            raise dnf.exceptions.Error("Directory {} not writable (check user permission) path={}".format(label, dirpath))
   
    def run(self):
        
        # check terminal leaf node and set dnf persisdir and copy environment to dnf conf
        node= reddnf.config.ParseNode(defaults=self.redconf, redpath=self.opts.redpath)
        node.CopyEnvTo(self.base.conf, "gpgcheck")
        node.CopyEnvTo(self.base.conf, "persistdir")

        # check persistant path exist and are writable
        self.checkdir("redpath", self.opts.redpath)
        self.checkdir("dnf persistdir", self.base.conf.persistdir, create=True)

        # Create an empty libsolv dependencies sack
        reddnf.sack.repo_create_empty(self.base, "@rpmdb")
        reddnf.sack.repo_load_rpmdb (self.base, "@rpmdb", self.opts.redpath)
        # avaliable repo with module only
        #reddnf.sack.repo_load_available (self.base, self.opts.redpath)
   
        # update dnf command argument list and run
        RedRemoveCommand._dnfcmd.opts = self.opts
        return (RedRemoveCommand._dnfcmd.run())
