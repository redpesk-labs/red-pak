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
class RedRepoListCommand(dnf.cli.Command):

    aliases = ['red-rlist', 'red-repolist', 'rlr', 'red-listrepo']
    summary = "Search rpm package in remote repo from terminal redpah leaf"
    _dnfcmd = None

    def __init__(self, cli):
        super(RedRepoListCommand, self).__init__(cli)
        self.opts = None
        self.parser = None
        RedRepoListCommand._dnfcmd =  dnf.cli.commands.repolist.RepoListCommand(cli)

    @staticmethod
    def set_argparser(parser):
        parser.add_argument("--redpath", help="container hierarchical rootfs path ex: /var/reddnf/platform/profile/project")
        RedRepoListCommand._dnfcmd.set_argparser(parser)

    # Never request any default dnf activation
    def configure(self):
        demands = self.cli.demands
        demands.sack_activation = False
        demands.root_user = False
        demands.available_repos = False
        self.confdflts=reddnf.defaults.values()

        # check we redpath is defined and exist
        if not self.opts.redpath:
            raise dnf.exceptions.Error("Syntax Error: reddnf --redpath=/xx/../xyz subcommand (missing --redpath)")

        if not os.path.isdir(self.opts.redpath):
            raise dnf.exceptions.Error("Error: Directory reddnf should exist [{}]".format(self.opts.redpath))

    def run(self):

        # Move default repodir to nodepath
        environment= reddnf.config.Environment(None)
        self.base.conf._set_value('reposdir', environment.ExpandValue('repodir', self.confdflts['REDNODE_REPODIR']))

        # update dnf command argument list and run
        RedRepoListCommand._dnfcmd.opts = self.opts
        RedRepoListCommand._dnfcmd.run()
