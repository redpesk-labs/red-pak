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


# Force redrpm before calling libdnf
import sys
import redrpm
sys.modules['rpm'] = sys.modules['redrpm']

import libdnf
import hawkey

#print ('Importing Fulup red-dnf')

# import in dependency order !!!
from . import _reddnf as _libso
from . import reddefaults as defaults
from . import redconfig as config
from . import sackload as sack

# Small Debug Helper
def dump(obj):
    import pprint
    import inspect
    pprint.pprint(inspect.getmembers(obj), indent=4, width=120)

