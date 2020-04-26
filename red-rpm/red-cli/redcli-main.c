/*
* Copyright (C) 2020 "IoT.bzh"
* Author Fulup Ar Foll <fulup@iot.bzh>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Documentation: https://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/ch-programming-c.html#id752589
                 http://ftp.rpm.org/max-rpm/s1-rpm-rpmlib-example-code.html
                 https://rikers.org/rpmbook/node113.html
*/
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <redrpm.h>

/* int rpmts_SolveCallback(rpmts ts, rpmds ds, const void * data) {

    const char* depName = rpmdsN(ds);
    const char* version = rpmdsEVR(ds);
    int deptype = rpmdsTagN(ds); // http://ftp.rpm.org/api/4.14.0/rpmds_8h.html#ab6d1a231818a4246cfe74976af7beda3
    int depFlag = rpmdsFlags(ds); // rpmsenseFlags rpmds.h

    printf ("rpmts_SolveCallback depname=%s version=%s\n", depName, version);
    return 0; 
}
 */

int main(int argc, char *argv[]) {
    mode_t mask = 0027;
    (void) umask(mask);
 
int rc;
}
/*
int status = rpmReadConfigFiles( (const char*) NULL, (const char*) NULL);

const char* rpmfile="/opt/tmp/flex-2.6.4-2.fc30.x86_64.rpm";
const char* rpmdbpath="/var/lib/rpm";
const char* pprefix="/profile";

if (status != 0) {
    printf("Error reading RC files.\n");
    goto OnErrorExit;
} 
printf("Read RPM config OK\n");

// create an empty transaction and get DBpointer
rpmts ts= rpmtsCreate(); 
rpmdb rdb= rpmtsGetRdb(ts);


// do not use rpm rootdir
rc = rpmtsSetRootDir(ts, "/");

// Read System RPMdb
rpmPushMacro(NULL, "_dbpath", NULL, rpmdbpath, 0);
rc = rpmtsOpenDB(ts, O_RDONLY);
if (rc) {
	rpmlog(RPMLOG_ERR, "*** fail to open RPMdb in /\n");
}

// Read profile RPMdb
{
    // need empry DB rpmdb --initdb --dbpath=/profile/var/lib/rpm/
    char tmpstring[255];
    strcat(tmpstring, pprefix);
    strcat(tmpstring, rpmdbpath);
    rpmPushMacro(NULL, "_dbpath", NULL, tmpstring, 0);
    rc = rpmtsOpenDB(ts, O_RDWR);
    if (rc) {
        rpmlog(RPMLOG_ERR, "*** fail to open RPMdb in /profile\n");
    } 
}

// force rpmlock in a location where we have write permission
rpmPushMacro(NULL, "_rpmlock_path", NULL, "/profile/var/lib/rpm/.rpm.lock", 0);

// Should loop on package to install 
// open RPM file
FD_t rpmfd = Fopen(argv[1],"r.ufdio"); 
if (rpmfd == NULL || Ferror(rpmfd)) {
    printf ("*** fail to open rpm=%s\n", argv[1]);
	goto OnErrorExit;
}

Header pkgHeader = NULL;
rpmRC rpmrc = rpmReadPackageFile(ts, rpmfd, rpmfile, &pkgHeader);
switch (rpmrc) {
    case RPMRC_NOTTRUSTED:
    case RPMRC_NOKEY:
    case RPMRC_OK:
	break;

    default:
	    fprintf(stderr, "rpmReadPackageInfo error=[%s]\n", strerror(errno));
	    goto OnErrorExit;
}

rpmRelocation relocation[2];
relocation[0].oldPath=NULL;
relocation[0].newPath="/profile";
relocation[1].oldPath=NULL;
relocation[1].newPath=NULL;

rc= rpmtsAddInstallElement(ts, pkgHeader, argv[1], 0, relocation);
// rc= rpmtsAddInstallElement(ts, pkgHeader, argv[1], 0, NULL);

// Set Solving callback
rc = rpmtsSetSolveCallback(ts, rpmts_SolveCallback, NULL);
if (rc) {
    rpmlog(RPMLOG_ERR, "*** Fail to register CB\n");
	goto OnErrorExit;
}

// run dependency check
printf ("Check Transaction Dependencies\n");
rc = rpmtsCheck(ts);
if (rc) {
    rpmlog(RPMLOG_ERR, "\n*** Missing dependencies\n");
	goto OnErrorExit;
}

// register runCB
rpmtsSetNotifyCallback(ts, rpmShowProgress, NULL);

printf ("Run Transaction Install\n");
// Everything looks OK install package
rc = rpmtsRun(ts, NULL, RPMPROB_FILTER_NONE );
if (rc) {
    rpmlog(RPMLOG_ERR, "\n*** Transaction Run Fail\n");
	goto OnErrorExit;
}
exit(0);

OnErrorExit:
    exit(1);
}  */