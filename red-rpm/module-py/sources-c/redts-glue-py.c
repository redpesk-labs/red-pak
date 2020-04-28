/*
 * Freely inspired from original librpm python interface
 *
 * Copyright (C) 2020 "IoT.bzh" and owner of python libRpm glue
 * Author Fulup Ar Foll <fulup@iot.bzh> and others
 *
 * GNU GENERAL PUBLIC LICENSE  Version 2, June 1991
 * Everyone is permitted to copy and distribute verbatim copies
 * of this license document, but changing it is not allowed.
 *
 */

/*
   this file hold method that have been modified from original librpm
   to support redpak hierarchical dependencies model.
*/

#include <fcntl.h>

#include <rpm/rpmspec.h>
#include <rpm/rpmbuild.h>
#include <rpm/rpmsign.h>

#include "redts-import-py.h"
#include "redts-glue-py.h"

// parse node config file to build install relocation datas
static rpmRelocation *redts_Relocate (rpmtsObject * s, redNodeT *node, RedConfDefaultsT *defaults) {
    redConfigT *config = node->config;
    rpmRelocation *relocation = calloc(config->relocations_count+2, sizeof(rpmRelocation));

    // if no relocation force prefix to redpath
    if (!config->relocations_count) {
        // redpath should replace root
        relocation[0].oldPath="/";
        relocation[0].newPath=(char*)s->redpak->redpath;
    } else for (int idx=0; idx < config->exports_count; idx++) {
        const char* old=config->relocations[idx].old;
        const char* new=config->relocations[idx].new;

        relocation[idx].oldPath= (char*)RedNodeStringExpand (node, defaults, old, NULL, NULL);
        relocation[idx].newPath= (char*)RedNodeStringExpand (node, defaults, new, NULL, NULL);
    }
    return relocation;
}

PyObject * redts_AddInstall(rpmtsObject * s, PyObject * args) {
    Header h = NULL;
    PyObject * key;
    int how = 0;
    int rc;

    // at this point we should have valid config
    if (!s->redpak->redtree) return NULL;

    s->redpak->shouldcheck= RED_CHECK_PARENT;

    if (!PyArg_ParseTuple(args, "O&Oi:AddInstall", hdrFromPyObject, &h, &key, &how))
	    return NULL;

    //printf("*** Fulup redts_AddInstall tsid=%d\n", s->tsid);
    rpmRelocation *relocation= redts_Relocate (s, s->redpak->redtree, nodeConfigDefaults);

    rc = rpmtsAddInstallElement(s->ts, h, key, how, relocation);
    if (key && rc == 0) {
    	PyList_Append(s->keyList, key);
    }
    return PyBool_FromLong((rc == 0));
}


PyObject *redts_AddReinstall(rpmtsObject * s, PyObject * args){
    Header h = NULL;
    PyObject * key;
    int rc;

    // at this point we should have valid config
    if (!s->redpak->redtree) return NULL;

    if (!PyArg_ParseTuple(args, "O&O:AddReinstall",
			  hdrFromPyObject, &h, &key))
	return NULL;

    printf("*** Fulup redts_ReInstall tsid=%d\n", s->tsid);
    rc = rpmtsAddReinstallElement(s->ts, h, key);
    if (rc == 0)  {
        s->redpak->shouldcheck=RED_CHECK_PARENT;
        if (key)
	        PyList_Append(s->keyList, key);
    }

    return PyBool_FromLong((rc == 0));
}

PyObject *redts_AddErase(rpmtsObject *s, PyObject * args) {
    Header h;
    int rc;

    // at this point we should have valid config
    if (!s->redpak->redtree) return NULL;

    if (!PyArg_ParseTuple(args, "O&:AddErase", hdrFromPyObject, &h))
        return NULL;

    printf("*** Fulup redts_AddErease tsid=%d\n", s->tsid);

    rc = rpmtsAddEraseElement(s->ts, h, -1);
    if (rc == 0)
        s->redpak->shouldcheck=RED_CHECK_PARENT;

    return PyBool_FromLong(rc == 0);
}

PyObject *redts_Run(rpmtsObject *s, PyObject *args, PyObject *kwds) {

    int rc;
    int verbose=s->redpak->redconf->conftag->verbose;
    struct rpmtsCallbackType_s cbInfo;
    rpmprobFilterFlags ignoreSet;
    char * kwlist[] = {"callback", "data", "ignoreSet", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOi:Run", kwlist, &cbInfo.cb, &cbInfo.data, &ignoreSet))
	    return NULL;

    // force relocation
    ignoreSet = ignoreSet | RPMPROB_FILTER_FORCERELOCATE;

    cbInfo.count= 0;
    cbInfo.tsid = s->tsid;
    cbInfo.tso = s;

    cbInfo._save = PyEval_SaveThread();

    // Set RPM verbosity level (3=ERR)
    rpmSetVerbosity(verbose+2);

    if (cbInfo.cb != NULL) {
        if (!PyCallable_Check(cbInfo.cb)) {
            PyErr_SetString(PyExc_TypeError, "expected a callable");
            return NULL;
        }
    	(void) rpmtsSetNotifyCallback(s->ts, rpmtsCallback, (void *) &cbInfo);
    }


    // if package need to install/update or check package we need to include ancessor rpmDBs
    if (s->redpak->shouldcheck == RED_CHECK_PARENT) {
        // at this point we should have valid config
        if (!s->redpak->redtree) {
            RedLog(REDLOG_ERROR, "redts_Run: Fail to config RedFamily from '%s'", s->redpak->redpath);
            return NULL;
        }

        //printf ("------- redts_Run tsid=%d running RedRegisterFamilyDb -----------\n", s->tsid);
        // load redpath cotresponding RpmDb
        rc = RedRegisterFamilyDb (s->ts, s->redpak->redconf, s->redpak->redtree);
        if (rc != 0) {
            RedLog(REDLOG_ERROR, "redts_Run: Fail to RpmDb RedFamily from '%s'", s->redpak->redpath);
            goto OnErrorExit;
        }
    }

    // Get rpm lock file from terminal node config
    // psuh lock from to redpath terminal leaf
    const char* lockfile = RedGetRpmVarDir (s->redpak->redconf, s->redpak->redtree, NULL, STATIC_STR_CONCAT("/", REDNODE_LOCK));
    rpmPushMacro(NULL, "_rpmlock_path", NULL, lockfile, 0);
    //printf ("------- redts_Run tsid=%d lockfile=%s -----------\n", s->tsid, lockfile);

    // this is where everything happen
    rc = rpmtsRun(s->ts, NULL, ignoreSet);

    // clear callback
    if (cbInfo.cb)
	    (void) rpmtsSetNotifyCallback(s->ts, NULL, NULL);

    PyEval_RestoreThread(cbInfo._save);

    // retreive status from transaction handle update timestamp and info zone
    redStatusT *redstatus= s->redpak->redtree->status;

   // save redpath terminal family leaf node
    rc= RedUpdateStatus(s->redpak->redtree, s->redpak->redconf->conftag->verbose);
    if (rc) {
        RedLog(REDLOG_ERROR, "Fail to update status rednode=%s ", s->redpak->redpath);
        goto OnErrorExit;
    }

    return Py_BuildValue("i", rc);

OnErrorExit:
    RedLog(REDLOG_ERROR, "(ABORT) redts_run fail");
    return NULL;
}


int redts_init(rpmtsObject *s, PyObject *args, PyObject *kwds) {
    const char *redpath;
    int done, warning;

    rpmVSFlags vsflags = rpmExpandNumeric("%{?__vsflags}");
    char * kwlist[] = {"rootdir", "vsflags", 0};

    // add redpak handle to transaction handle
    s->redpak = calloc (1, sizeof(redpakTsT));

    // redpak stick rootdir to '/'
    (void) rpmtsSetRootDir(s->ts, "/");
    (void) rpmtsSetVSFlags(s->ts, vsflags);

    // retreive redpath from rootdir DNF arguments
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|si:rpmts_new", kwlist,  &redpath, &vsflags)) {
        RedLog(REDLOG_ERROR, "redts_init: Invalid arguments usage RedTransaction(redpath) ");
	    goto OnErrorExit;
    }

    // get redpak root config from env/default path
    const char *redConfPath = getenv("redpak_MAIN");
    if (!redConfPath) redConfPath= redpak_MAIN;
    const char *warningStr = getenv("redpak_WARN");
    if (!warningStr) {
        warning=RED_CONFIG_WARNING_DEFAULT;
    } else {
        done= sscanf (warningStr,"%d", &warning);
        if (!done)  warning=RED_CONFIG_WARNING_DEFAULT;
    }

    s->redpak->redconf = RedLoadConfig (redConfPath, warning);
    if (!s->redpak->redconf) {
        RedLog(REDLOG_ERROR, "redts_init: fail to parse redpak main config at:'%s' ", redConfPath);
	    goto OnErrorExit;
    }

    // rootdir = '/' stop here otherwise scandown redpath node tree
    if (redpath[0] == '/' && redpath[1] == '\0') {
        s->redpak->redtree = NULL;
        return 0;
    } else {
        s->redpak->redpath= strdup(redpath);
    }

    // if warning env was not set use the one from main config
    if (!warningStr) warning=s->redpak->redconf->conftag->verbose;

    // Load Family node tree
    s->redpak->redtree = RedNodesScan(s->redpak->redpath, warning);
    if (!s->redpak->redtree) {
        RedLog(REDLOG_WARNING, "Fail to scan rednodes family tree redpath=%s ", s->redpak->redpath);
    }


    return 0;

OnErrorExit:
    PyErr_SetString(pyrpmError, "(ABORT) redts_init fail creating core transaction object");
    return -1;

}
