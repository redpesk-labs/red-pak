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

# /opt/bin/dnf-3 -c /opt/etc/dnf/redpack.yaml red-node --redpath=/var/redpak/agl-master --create --alias=xxx

import os
import dnf
import reddnf

@dnf.plugin.register_command
class RedManagerCommand(dnf.cli.Command):

    aliases = ['red-manager','rmg']
    summary = "Create/Delete redpak rootpath leaves"
    _dnfcmd = None

    def __init__(self, cli):
        super(RedManagerCommand, self).__init__(cli)
        self.opts = None
        self.parser = None
        RedManagerCommand._dnfcmd = cli.cli_commands["config-manager"](cli)

    @staticmethod
    def set_argparser(parser):
        parser.add_argument("--redpath", help="Terminal rednode leaf example:/var/redpak/platform/profile/project")
        parser.add_argument("--alias", help="rednode alias name [mandatory when creating")
        parser.add_argument("--update", default=False, action='store_true', help="force creation even when node exist")
        parser.add_argument("--create", default=False, action='store_true', help="Create an empry node [require --alias]")
        parser.add_argument("--template", help="create node config from temmplate [default= /etc/redpak/template.d/default.yaml]")
        RedManagerCommand._dnfcmd.set_argparser(parser)

    def configure(self):
        demands = self.cli.demands
        demands.sack_activation = False
        demands.root_user = False
        demands.available_repos = False
        self.nodeconfig=False
        self.confdflts=reddnf.defaults.values()

        if self.opts.update:
            self.opts.create=True

        if self.opts.create:
            self.nodeconfig=True
            if not self.opts.alias:
               raise dnf.exceptions.Error("--create require --alias=xxxx [no default]")
            if not self.opts.template:
               self.opts.template='default' 

        # check we redpath is defined and exist
        if not self.opts.redpath:
            raise dnf.exceptions.Error("Syntax Error: redpak --redpath=/xx/../xyz subcommand (missing --redpath)")

        if (not self.nodeconfig and not (self.opts.add_repo != [] or
            self.opts.save or
            self.opts.dump or
            self.opts.dump_variables or
            self.opts.set_disabled or
            self.opts.set_enabled) ):
                    self.cli.optparser.error("one of the following arguments is required: {}".format(' '.join([
                    "--save", "--add-repo", "--dump", "--dump-variables", "--enable", "--disable"])))   

    def checkdir(self, label, dirpath, create=False):

        if create:
            try:
                os.makedirs(dirpath)
            except FileExistsError:
                pass
            except OSError:
                raise dnf.exceptions.Error("Directory {} fail to create path={}".format(label, dirpath))

        if not os.access(dirpath, os.W_OK):
            raise dnf.exceptions.Error("Directory {} not writable (check user permission) path={}".format(label, dirpath))        

    def CreateRednode(self):

        # if redpath does not exist create it now
        self.checkdir("redpath", self.opts.redpath, create=True)
   
        reddnf.config.rednode_template (self.opts, None)
        reddnf.config.rednode_status (self.opts, None)

    def run(self):

        if self.opts.create:
            self.CreateRednode ()
        
        # if node config stop here
        if (self.nodeconfig):
            print("-----------------------------------------------------------")
            print("SUCCESS: rednode ready at {}".format(self.opts.redpath))
            print("-----------------------------------------------------------")
            return

        # at this point redpath should exist
        self.checkdir("redpath", self.opts.redpath)
    
        # check redpath is writable
        if not os.access(self.opts.redpath, os.W_OK):
            raise dnf.exceptions.Error("Redpath not writable redpath=/xx/../xyz (check user permission)")

        # Move default repodir to nodepath
        #self.base.conf._set_value('reposdir', environment.ExpandValue('repodir', self.confdflts['REDNODE_REPODIR']))

        # Load avaliable repo
        dnf.conf.installroot=self.opts.redpath
        reddnf.sack.repo_create_empty(self.base, "@rpmdb")
        reddnf.sack.repo_load_available (self.base, self.opts.redpath)

        # Load parsing options in command object and runit
        RedManagerCommand._dnfcmd.opts = self.opts
        RedManagerCommand._dnfcmd.run()
