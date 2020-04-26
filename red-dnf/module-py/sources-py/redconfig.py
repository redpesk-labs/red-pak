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
import os
import dnf
import yaml
from string import Template
import reddnf

class Environment():
    def __init__(self, defaults=None): 
       self.defaults=defaults

    def ExpandValue (self, label, value):

        # if value is not a sting then do not expand it
        if not isinstance (value, str):
            return value

        # Try to expand from environement variables
        try:
            value = Template(value).substitute(os.environ)
            done =True
        except:
            done=False

        # IF not in environment use defaults
        if not done:
            for key in self.defaults:
                if isinstance(self.defaults[key], str):
                    token = '${}'.format(key.upper())
                    value=value.replace(token, self.defaults[key])

        return value

class ParseNode(Environment):

    def __init__(self, defaults=None, redpath=None, strictmod=1):
        self.strictmod=strictmod
        self.defaults=defaults
        if not self.defaults:
            self.defaults={}

        # If we have no redpath or when redpath=/ than we seach for coreos redpak.conf otherwise search for a rednode
        if redpath and redpath != '/':
            self.confpath= '{}/etc/redpak.conf'.format(redpath)
            self.defaults['redpath']=redpath
            self.GetConf()
        else:
            self.defaults['redpath']='/'
            self.GetMain() 

    def HasConf(self):
        return self.isnode

    def GetConf(self):
        if not os.path.exists(self.confpath):
            if  self.strictmod:     
                raise dnf.exceptions.Error("No file at path={}".format(self.confpath))     
            else:
                self.isnode=False
                print ("Info: Node ignored [no redpak.conf] path={} ()".format(self.confpath))
                return
        else:
            self.isnode=True

        with open(self.confpath, "r") as Environment:
            config = yaml.load(Environment, Loader=yaml.FullLoader)

        if not config:
            raise dnf.exceptions.Error("No redpak config at path={}".format(self.confpath))

        if not 'headers' in config:
            raise dnf.exceptions.Error("No 'headers' sections in config at path={}".format(self.confpath))

        if not 'config' in config:
            raise dnf.exceptions.Error("No 'environment' sections in config at path={}".format(self.confpath))

        self.headers = config['headers']
        self.config = config['config']

    # get main /etc/repack/main.conf and set umask default value
    def GetMain(self):

        if 'redpak_MAIN' in os.environ:
            self.confpath= os.environ['redpak_MAIN']
        elif 'redpak_MAIN' in self.defaults:
            self.confpath=self.defaults['redpak_MAIN']
        else:
            self.confpath='/etc/redpak/main.conf'

        self.GetConf()       
        if not 'umask' in self.config:
            os.umask(0o077)
        else:
            umask = self.config["umask"]
            os.umask(umask)

        return  

    def GetEnv (self, label=None):
        if not self.config:
            raise dnf.exceptions.Error("No 'environment' sections in config at path={}".format(self.confpath))

        if not label:
            return self.config
        else:
            if label in self.config:  
                return self.config[label]
            else:
                return None    


    def GetHeaders (self, label):
        if not hasattr (self, 'headers'):
            # Not having a '.rednode_status' only make node to be ignored
            return None

        if not label:
            return self.headers
        else:
            if label in self.headers:
                return self.headers[label]
            else:
                return None 
             

    def CopyEnvTo (self, baseconf, label):
        if not self.config:
            raise dnf.exceptions.Error("No 'environment' sections in config at path={}".format(self.confpath))

        if label:
            if label in self.config:
                value= self.config[label]
                value = self.ExpandValue (label, value)
                setattr(baseconf, label, value) 
        else:
            for keyname in self.config:
                    value= self.ExpandValue (keyname, self.config[keyname])
                    try:
                        # some attribute are readonly
                        setattr(baseconf, keyname, value) 
                    except:
                        print ("Warning ignoring config {}={}".format(keyname, value))
                        pass    



# split redpath and return a table composed of a repo label and leaf fullpath
def redpath_split(redpath):
    dirlist = []
    fullpath= ''
    repname=''

    if not os.access(redpath, os.R_OK):
        raise dnf.exceptions.Error("Redpath not readable path={} (check user permission)".format(redpath))

    # scan redpath and return the hierachie of contained fullpath
    dirtoken = redpath.split('/')
    for dirname in dirtoken:

        # special rootdir processing
        if not dirname:
            dirlist.append(['@red-coreos', '/'])
            continue

        # If not a valid node just ignore
        fullpath= '{}/{}'.format(fullpath, dirname)
        rednode= ParseNode(redpath=fullpath, strictmod=0)
        if not rednode.HasConf():
            continue
          
        # build redpath hierarchie
        alias= rednode.GetHeaders('alias')
        if not alias:
            raise dnf.exceptions.Error("Redpath not a valid node (no alias) path={}".format(rednode.confpath))

        repname= "@red-{}".format(alias)

        if not os.access(redpath, os.R_OK):
            raise dnf.exceptions.Error("Redpath not readable path={} (check user permission)".format(fullpath))

        dirlist.append([repname, fullpath])

    return dirlist    


class ParseTemplate(Environment):

    def __init__(self, tmplname, defaults):

        if not defaults:
            defaults={}
            defaults['redpak_TMPL']= "/etc/redpak/templates.d"
            
        self.defaults=defaults
        self.tmplpath='{}/{}.conf'.format(defaults['redpak_TMPL'], tmplname)

        if not os.access(self.tmplpath, os.R_OK):
            raise dnf.exceptions.Error("Template not readable path={}".format(self.tmplpath))

        try:    
            with open(self.tmplpath, "r") as redtmpl:
                template = yaml.load(redtmpl, Loader=yaml.FullLoader)
        except:
            raise dnf.exceptions.Error("No/Invalid redpak config at path={}".format(self.tmplpath))

        if not 'headers' in template:
            raise dnf.exceptions.Error("No 'headers' sections in config at path={}".format(self.tmplpath))

        # expand headers    
        for section in template:
            if isinstance (template[section], list):
                for idx, item in enumerate(template[section]):
                    for keyname in item:
                        value= self.ExpandValue (keyname, item[keyname])
                        template[section][idx][keyname]=value

            elif isinstance (template[section], dict):
                for keyname in template[section]:
                    value= self.ExpandValue (keyname, template[section][keyname])
                    template[section][keyname]=value
            else:
                raise dnf.exceptions.Error("Invalid redpak template tmplpath={}".format(self.tmplpath))

        self.template=template

    def saveto (self, opts):

        confpath= self.ExpandValue ('template', self.defaults['REDNODE_CONF'])
        confdir= os.path.dirname(confpath)
        if not os.path.isdir(confdir):
            try:
                os.makedirs(confdir)
            except:
                raise dnf.exceptions.Error("Fail to create rednode config at confpath={}".format(confdir))

        if not opts.update and os.path.exists(confpath):
            raise dnf.exceptions.Error("Rednode config already exist [use --update] confpath={}".format(confdir))

        try:
            with open(confpath, "w") as conffile:
                yaml.dump(self.template, conffile, default_flow_style=False) 
        except:
            raise dnf.exceptions.Error("Fail to push rednode config at path={}".format(confpath))

# take a template from /etc/redpak/templates.d expand it and saveit to node config
def rednode_template (opts, srcdefaults):
    import uuid
    import datetime
    
    if not srcdefaults:
        srcdefaults = reddnf.defaults.values()

    now=datetime.datetime.now()

    # build date string
    now= datetime.datetime.now()
    # push env var for template expansion within config files
    os.environ['NODE_ALIAS']= opts.alias
    os.environ['NODE_UUID']= str(uuid.uuid1())
    os.environ['TODAY']=now.strftime("%d:%b:%Y %H:%M (%Z)")

    template= ParseTemplate(opts.template, srcdefaults)
    template.saveto(opts)

# take a template from /etc/redpak/templates.d expand it and saveit to node config
def rednode_status (opts, srcdefaults):
    import time
    import datetime

    if not srcdefaults:
        srcdefaults= reddnf.defaults.values()

    # current time in ms
    timems = int(round(time.time() * 1000))
    now=datetime.datetime.now()

    # create default node status
    status={}
    status['state']    = 'Enable'
    status['realpath'] = opts.redpath
    status['timestamp']= timems
    status['info']     = 'created by red-manager the {}'.format(now.strftime("%d:%b:%Y %H:%M (%Z)"))

    environment= Environment(None)
    confpath= environment.ExpandValue('rednode.status', srcdefaults['REDNODE_STATUS'])

    try:
        with open(confpath, "w") as conffile:
                yaml.dump(status, conffile, default_flow_style=False) 
    except:
        raise dnf.exceptions.Error("Fail to push rednode status at path={}".format(confpath))


# Native dict merge only appear in Py-v3.5
def merge_two_dicts(x, y):
    z = x.copy()
    z.update(y)
    return z

# Merge pkgdefault with node environment
def rednode_defaults (opts, srcdefaults):
  
    if not srcdefaults:
       srcdefaults= reddnf.defaults.values()

    # get node environment
    node= ParseNode(redpath=opts.redpath)

    # This only work with python > 3.5 (but I'm lazy for not using it)
    defaults = srcdefaults.copy() 
    defaults.update(node.config)
    return defaults    
