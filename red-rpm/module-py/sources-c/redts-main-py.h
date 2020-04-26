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

#ifndef _INCLUDE_REDTS_PY_
#define _INCLUDE_REDTS_PY_

#include <Python.h>
#include <pyerrors.h>
#include <unistd.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmfiles.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmts.h>

#include "redrpm.h"

#define FULUP_TRACER(var)  fprintf (stderr,"*** [%s] Func=%s File=%s Line=%d ***\n", var, __FUNCTION__, __FILE__ , __LINE__);


// global error object
extern PyObject* pyrpmError;

typedef enum {
    RED_CHECK_NO=0,
    RED_CHECK_PARENT,
    RED_CHECK_CHILDREN,
} redpakCheckE;

// place holder for every redpak config and data
typedef struct {
    const char *redpath;  // redpath terminal leaf path
    redNodeT *redtree;    // redpath full family node tree
    redConfigT *redconf;   // main config from /etc/redpak/main.yaml
    redpakCheckE shouldcheck; // status passed from install/update/remove to transaction run
} redpakTsT;

// transaction object hold a list of action to do for a given transaction
typedef struct rpmtsObject_s {
    PyObject_HEAD
    PyObject *md_dict;
    void *scriptFd;
    PyObject *keyList;
    rpmts	  ts;
    rpmtsi    tsi;
    redpakTsT *redpak;
    int tsid;
} rpmtsObject;

#endif  