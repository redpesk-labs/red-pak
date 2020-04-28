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


#ifndef _INCLUDE_RPM_IMPORT_PY_
#define _INCLUDE_RPM_IMPORT_PY_

#include "redts-main-py.h"

#include "rpmlib-ori/header-py.h"
#include "rpmlib-ori/rpmkeyring-py.h"
#include "rpmlib-ori/rpmkeyring-py.h"
#include "rpmlib-ori/rpmfd-py.h"
#include "rpmlib-ori/rpmps-py.h"
#include "rpmlib-ori/rpmmi-py.h"
#include "rpmlib-ori/rpmii-py.h"
#include "rpmlib-ori/rpmte-py.h"
#include "rpmlib-ori/rpmfiles-py.h"
#include "rpmlib-ori/rpmstrpool-py.h"
#include "rpmlib-ori/rpmds-py.h"
#include "rpmlib-ori/rpmfi-py.h"
#include "rpmlib-ori/rpmmacro-py.h"
#include "rpmlib-ori/rpmfd-py.h"

struct rpmtsCallbackType_s {
    PyObject *cb;
    PyObject *data;
    rpmtsObject *tso;
    PyThreadState *_save;
    int count;
    int tsid;
};

int ModAddElements(PyObject *module) ;

// Transaction methods
PyObject *rpmts_getKeyring(rpmtsObject *s, PyObject *args, PyObject *kwds);
PyObject * rpmts_AddInstall(rpmtsObject * s, PyObject * args);
PyObject *rpmts_AddReinstall(rpmtsObject * s, PyObject * args);
PyObject *rpmts_AddErase(rpmtsObject * s, PyObject * args);
PyObject *rpmts_HdrFromFdno(rpmtsObject * s, PyObject *arg);
PyObject * rpmts_HdrCheck(rpmtsObject * s, PyObject *obj);
PyObject *rpmts_PgpPrtPkts(rpmtsObject * s, PyObject * args, PyObject * kwds);
PyObject *rpmts_PgpImportPubkey(rpmtsObject * s, PyObject * args, PyObject * kwds);
PyObject *rpmts_setKeyring(rpmtsObject *s, PyObject *arg);
PyObject *rpmts_getKeyring(rpmtsObject *s, PyObject *args, PyObject *kwds);

PyObject *rpmts_Check(rpmtsObject * s, PyObject * args, PyObject * kwds);
PyObject *rpmts_Order(rpmtsObject * s);
PyObject *rpmts_Clean(rpmtsObject * s);
PyObject *rpmts_Clear(rpmtsObject * s);

PyObject *rpmts_Problems(rpmtsObject * s);
PyObject *rpmts_Run(rpmtsObject * s, PyObject * args, PyObject * kwds);

// Module methods
PyObject * doLog(PyObject * self, PyObject * args, PyObject *kwds);
PyObject * setLogFile (PyObject * self, PyObject *arg);

PyObject * utf8FromString(const char *s);

void *rpmtsCallback(const void * hd, const rpmCallbackType what,
		         const rpm_loff_t amount, const rpm_loff_t total,
	                 const void * pkgKey, rpmCallbackData data);

PyObject *rpmts_OpenDB(rpmtsObject * s);
PyObject *rpmts_CloseDB(rpmtsObject * s);
PyObject * rpmts_InitDB(rpmtsObject * s);
PyObject * rpmts_RebuildDB(rpmtsObject * s);
// PyObject * rpmts_dbCookie(rpmtsObject * s);
PyObject *rpmts_VerifyDB(rpmtsObject * s);
PyObject * rpmts_Match(rpmtsObject * s, PyObject * args, PyObject * kwds);
PyObject *rpmts_index(rpmtsObject * s, PyObject * args, PyObject * kwds);
PyObject *rpmts_iternext(rpmtsObject * s) ;

PyObject *rpmts_get_tid(rpmtsObject *s, void *closure);

PyObject *rpmts_get_color(rpmtsObject *s, void *closure);
PyObject *rpmts_get_prefcolor(rpmtsObject *s, void *closure) ;
int rpmts_set_color(rpmtsObject *s, PyObject *value, void *closure);
int rpmts_set_prefcolor(rpmtsObject *s, PyObject *value, void *closure);
PyObject *setVerbosity (PyObject * self, PyObject * arg);

void addRpmTags(PyObject *module);
PyObject * addSign(PyObject * self, PyObject * args, PyObject *kwds);
PyObject * delSign(PyObject * self, PyObject * args, PyObject *kwds);

PyObject *rpmts_get_rootDir(rpmtsObject *s, void *closure);
PyObject *rpmts_get_flags(rpmtsObject *s, void *closure);
int rpmts_set_flags(rpmtsObject *s, PyObject *value, void *closure);
PyObject *rpmts_get_vsflags(rpmtsObject *s, void *closure);
int rpmts_set_vsflags(rpmtsObject *s, PyObject *value, void *closure);

#endif
