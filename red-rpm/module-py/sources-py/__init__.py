#
# Freely inspired from original librpm python interface
#
# Copyright (C) 2020 "IoT.bzh" and owner of python libRpm glue
# Author Fulup Ar Foll <fulup@iot.bzh> and others
#
# GNU GENERAL PUBLIC LICENSE  Version 2, June 1991
# Everyone is permitted to copy and distribute verbatim copies
# of this license document, but changing it is not allowed.
#
#


from __future__ import absolute_import
from sys import version_info as python_version
import warnings

# Small Debug Helper
def dump(obj):
    import pprint
    import inspect
    pprint.pprint(inspect.getmembers(obj), indent=4, width=120)

# try make redrpm look like original rpm module
from redrpm._redrpm import *
from redrpm.transaction import *
import redrpm._redrpm as _rpm

_RPMVSF_NODIGESTS = _rpm._RPMVSF_NODIGESTS
_RPMVSF_NOHEADER = _rpm._RPMVSF_NOHEADER
_RPMVSF_NOPAYLOAD = _rpm._RPMVSF_NOPAYLOAD
_RPMVSF_NOSIGNATURES = _rpm._RPMVSF_NOSIGNATURES

__version__ = _rpm.__version__
__version_info__ = tuple(__version__.split('.'))

