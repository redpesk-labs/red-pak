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

# Default DNF sack class does not allow to separate system and avaliable repo load
# This class spilt fil_sack into multiple method to allow multiple rpmdb to be loaded
# Check dnf/base.py:fill_sack for references
# b /opt/lib64/python3.7/site-packages/reddnf/sackload.py:39

import os
import dnf
import time
import datetime
import reddnf
import logging
import yaml

logger = logging.getLogger("dnf")
       

def repo_create_empty (base, reponame):
    timer = dnf.logging.Timer('create_empty_sack ****')
    base.reset(sack=True, goal=True)
    base._sack = dnf.sack._build_sack(base)
    lock = dnf.lock.build_metadata_lock(base.conf.cachedir, base.conf.exit_on_lock)
    with lock:
        reddnf._libso._repo_create_empty(base.sack, reponame)
    timer()
    return 

def repo_load_rpmdb (base, reponame, nodedir):
    timer = dnf.logging.Timer('create_empty_sack ****')
    lock = dnf.lock.build_metadata_lock(base.conf.cachedir, base.conf.exit_on_lock)
    with lock:
        reddnf._libso._repo_load_rpmdb(base.sack, reponame, nodedir)
    timer()
    return 

# modified fill_sack method from dnf/base.py
def repo_load_available(base, nodedir):
    error_repos = []
    mts = 0
    age = time.time()
    conf = base.conf
    
    timer = dnf.logging.Timer('sack_load_available_repos repo ****')
    reddnf._libso._repo_set_rootdir(base.sack, nodedir)
    lock = dnf.lock.build_metadata_lock(base.conf.cachedir, base.conf.exit_on_lock)
    with lock:
        # Iterate over installed GPG keys and check their validity using DNSSEC
        if conf.gpgkey_dns_verification:
            dnf.dnssec.RpmImportedKeys.check_imported_keys_validity()
        for r in base.repos.iter_enabled():
            try:
                base._add_repo_to_sack(r)
                if r._repo.getTimestamp() > mts:
                    mts = r._repo.getTimestamp()
                if r._repo.getAge() < age:
                    age = r._repo.getAge()
                logger.debug("%s: using metadata from %s.", r.id,
                                dnf.util.normalize_time(
                                    r._repo.getMaxTimestamp()))
            except dnf.exceptions.RepoError as e:
                r._repo.expire()
                if r.skip_if_unavailable is False:
                    raise
                logger.warning("Error: %s", e)
                error_repos.append(r.id)
                r.disable()
        if error_repos:
            logger.warning("Ignoring repositories: %s", ', '.join(error_repos))
        if base.repos._any_enabled():
            if age != 0 and mts != 0:
                logger.info("Last metadata expiration check: %s ago on %s.",
                            datetime.timedelta(seconds=int(age)),
                            dnf.util.normalize_time(mts))

        # Force final RPM installation and force destination
        base.conf.installroot=nodedir
        base._sack._configure(base.conf.installonlypkgs)
        base._goal = dnf.goal.Goal(base._sack)
    return