
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

# Fulup Note: support on python3 and change _rpm name space to avoid conflict
# with original librpm Python API.

import sys
import redrpm 
from redrpm._redrpm import ts as TransactionSetCore

class TransactionSet(TransactionSetCore):
    _probFilter = 0

    def __init__(self, rootdir=None):
        #print ("*** Fulup New TransactionSet rootdir=", rootdir)
        super(TransactionSet, self).__init__(rootdir)

    def _wrapSetGet(self, attr, val):
        oval = getattr(self, attr)
        setattr(self, attr, val)
        return oval

    def setVSFlags(self, flags):
        return self._wrapSetGet('_vsflags', flags)

    def getVSFlags(self):
        return self._vsflags

    def setFlags(self, flags):
        return self._wrapSetGet('_flags', flags)

    def setProbFilter(self, ignoreSet):
        return self._wrapSetGet('_probFilter', ignoreSet)

    def getKeys(self):
        keys = []
        for te in self:
            keys.append(te.Key())
        # Backwards compatibility goo - WTH does this return a *tuple* ?!
        if not keys:
            return None
        else:
            return tuple(keys)

    def _f2hdr(self, item):
        if isinstance(item, str):
            with open(item) as f:
                header = self.hdrFromFdno(f)
        elif isinstance(item, redrpm.hdr):
            header = item
        else:
            header = self.hdrFromFdno(item)
        return header

    def addInstall(self, item, key, how="u"):

        header = self._f2hdr(item)

        if how not in ['u', 'i']:
            raise ValueError('how argument must be "u" or "i"')
        upgrade = (how == "u")

        if not TransactionSetCore.addInstall(self, header, key, upgrade):
            raise redrpm.error("adding package to transaction failed")

    def addReinstall(self, item, key):
        header = self._f2hdr(item)

        if not TransactionSetCore.addReinstall(self, header, key):
            raise redrpm.error("adding package to transaction failed")

    def addErase(self, item):
        hdrs = []
        if isinstance(item, redrpm.hdr):
            hdrs = [item]
        elif isinstance(item, redrpm.mi):
            hdrs = item
        elif isinstance(item, int):
            hdrs = self.dbMatch(redrpm.RPMDBI_PACKAGES, item)
        elif isinstance(item, str):
            hdrs = self.dbMatch(redrpm.RPMDBI_LABEL, item)
        else:
            raise TypeError("invalid type %s" % type(item))

        for h in hdrs:
            if not TransactionSetCore.addErase(self, h):
                raise redrpm.error("package not installed")

        # garbage collection should take care but just in case...
        if isinstance(hdrs, redrpm.mi):
            del hdrs

    def run(self, callback, data):
        #print ("*** Fulup run red transaction.py rootdir=", self.rootDir)
        rc = TransactionSetCore.run(self, callback, data, self._probFilter)

        # crazy backwards compatibility goo: None for ok, list of problems
        # if transaction didn't complete and empty list if it completed
        # with errors
        if rc == 0:
            return None

        res = []
        if rc > 0:
            for prob in self.problems():
                item = ("%s" % prob, (prob.type, prob._str, prob._num))
                res.append(item)

        return res

    def check(self, *args, **kwds):
        TransactionSetCore.check(self, *args, **kwds)

        # compatibility: munge problem strings into dependency tuples of doom
        res = []
        for p in self.problems():
            # is it anything we need to care about?
            if p.type == redrpm.RPMPROB_CONFLICT:
                sense = redrpm.RPMDEP_SENSE_CONFLICTS
            elif p.type == redrpm.RPMPROB_REQUIRES:
                sense = redrpm.RPMDEP_SENSE_REQUIRES
            else:
                continue

            # strip arch, split to name, version, release
            nevr = p.altNEVR.rsplit('.', 1)[0]
            n, v, r = nevr.rsplit('-', 2)

            # extract the dependency information
            needs = p._str.split()
            needname = needs[0]
            needflags = redrpm.RPMSENSE_ANY
            if len(needs) == 3:
                needop = needs[1]
                if '<' in needop:
                    needflags |= redrpm.RPMSENSE_LESS
                if '=' in needop:
                    needflags |= redrpm.RPMSENSE_EQUAL
                if '>' in needop:
                    needflags |= redrpm.RPMSENSE_GREATER
                needver = needs[2]
            else:
                needver = ""

            res.append(((n, v, r),
                        (needname, needver), needflags, sense, p.key))

        return res

    def hdrCheck(self, blob):
        res, msg = TransactionSetCore.hdrCheck(self, blob)
        # generate backwards compatibly broken exceptions
        if res == redrpm.RPMRC_NOKEY:
            raise redrpm.error("public key not available")
        elif res == redrpm.RPMRC_NOTTRUSTED:
            raise redrpm.error("public key not trusted")
        elif res != redrpm.RPMRC_OK:
            raise redrpm.error(msg)

    def hdrFromFdno(self, fd):
        res, h = TransactionSetCore.hdrFromFdno(self, fd)
        # generate backwards compatibly broken exceptions
        if res == redrpm.RPMRC_NOKEY:
            raise redrpm.error("public key not available")
        elif res == redrpm.RPMRC_NOTTRUSTED:
            raise redrpm.error("public key not trusted")
        elif res != redrpm.RPMRC_OK:
            raise redrpm.error("error reading package header")

        return h
