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
class RedListCommand(dnf.cli.Command):

    aliases = ['red-list', 'rls']
    summary = "List obsoletes/recent/extra packet from redpath terminal leaf (usefull for manual update)"
    _dnfcmd = None

    def __init__(self, cli):
        super(RedListCommand, self).__init__(cli)
        self.opts = None
        self.parser = None
        RedListCommand._dnfcmd =  dnf.cli.commands.ListCommand(cli)
        
    @staticmethod
    def set_argparser(parser):
        parser.add_argument("--redpath", help="container hierarchical rootfs path ex: /var/redpak/platform/profile/project")
        RedListCommand._dnfcmd.set_argparser(parser)

    # Never request any default dnf activation
    def configure(self):
        demands = self.cli.demands
        demands.sack_activation = False
        demands.root_user = False
        demands.available_repos = False

        # check we redpath is defined and exist
        if not self.opts.redpath:
            raise dnf.exceptions.Error("Syntax Error: redpak --redpath=/xx/../xyz subcommand (missing --redpath)")

        if not os.path.isdir(self.opts.redpath):
            raise dnf.exceptions.Error("Error: Directory redpak should exist [{}]".format(self.opts.redpath))

        # Merge source default with main config environment
        self.redconf=reddnf.config.rednode_defaults(self.opts, reddnf.defaults.values())
   
    def run(self):

        # check terminal leaf node and set dnf persisdir and copy environment to dnf conf
        node= reddnf.config.ParseNode(defaults=self.redconf, redpath=self.opts.redpath)
        node.CopyEnvTo(self.base.conf, "gpgcheck")
        node.CopyEnvTo(self.base.conf, "persistdir")

        # Create an empty libsolv dependencies sack
        reddnf.sack.repo_create_empty(self.base, "@rpmdb")

        # finally add avaliable remote repo from redpath termination leaf
        reddnf.sack.repo_load_rpmdb (self.base, "@System.solv", self.opts.redpath)

        # update dnf command argument list and run
        RedListCommand._dnfcmd.opts = self.opts
        RedListCommand._dnfcmd.run()
