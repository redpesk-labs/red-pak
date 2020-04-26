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
    This file containes almost unmodified functions from original rpmLib 
    mostly rpmts.c and rpmodule.c. Only routines needed for DNF where imported
*/


#include <fcntl.h>
#include "redts-main-py.h"

#include <rpm/rpmspec.h>
#include <rpm/rpmbuild.h>
#include <rpm/rpmsign.h>


#include "redts-doc.inc"
#include "redts-import-py.h"


RPM_GNUC_NORETURN
static void die(PyObject *cb) {
    char *pyfn = NULL;
    PyObject *r;

    if (PyErr_Occurred()) {
	PyErr_Print();
    }
    if ((r = PyObject_Repr(cb)) != NULL) { 
	pyfn = PyBytes_AsString(r);
    }
    fprintf(stderr, "FATAL ERROR: python callback %s failed, aborting!\n", 
	    	      pyfn ? pyfn : "???");
    exit(EXIT_FAILURE);
}

PyObject * rpmts_Match(rpmtsObject *s, PyObject *args, PyObject *kwds) {
    PyObject *KeyPy = NULL;
    PyObject *str = NULL;
    PyObject *mio = NULL;
    char *key = NULL;
    int lkey = 0;
    int len = 0;
    rpmDbiTagVal tag = RPMDBI_PACKAGES;
    char * kwlist[] = {"tagNumber", "key", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O:Match", kwlist, tagNumFromPyObject, &tag, &KeyPy))
	return NULL;

    if (KeyPy) {
	if (PyLong_Check(KeyPy)) {
	    lkey = PyLong_AsLong(KeyPy);
	    key = (char *)&lkey;
	    len = sizeof(lkey);
	} else if (PyLong_Check(KeyPy)) {
	    lkey = PyLong_AsLong(KeyPy);
	    key = (char *)&lkey;
	    len = sizeof(lkey);
	} else if (utf8FromPyObject(KeyPy, &str)) {
	    key = PyBytes_AsString(str);
	    len = PyBytes_Size(str);
	} else {
	    PyErr_SetString(PyExc_TypeError, "unknown key type");
	    return NULL;
	}
	//One of the conversions above failed, exception is set already 
	if (PyErr_Occurred()) goto exit;
    }

    // XXX If not already opened, open the database O_RDONLY now. 
    // XXX FIXME: lazy default rdonly open also done by rpmtsInitIterator(). 
    if (rpmtsGetRdb(s->ts) == NULL) {
        int rc = rpmtsOpenDB(s->ts, O_RDONLY);
        if (rc || rpmtsGetRdb(s->ts) == NULL) {
            PyErr_SetString(pyrpmError, "rpmdb open failed");
            goto exit;
        }
    } 
    rpmdbMatchIterator mi= rpmtsInitIterator(s->ts, tag, key, len);
    mio = rpmmi_Wrap(&rpmmi_Type, mi, (PyObject*)s);

exit:
    Py_XDECREF(str);
    return mio;
}

PyObject *rpmts_index(rpmtsObject * s, PyObject * args, PyObject * kwds){

    rpmDbiTagVal tag;
    PyObject *mio = NULL;
    char * kwlist[] = {"tag", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:Keys", kwlist,
              tagNumFromPyObject, &tag))
	return NULL;

    /* XXX If not already opened, open the database O_RDONLY now. */
    if (rpmtsGetRdb(s->ts) == NULL) {
	int rc = rpmtsOpenDB(s->ts, O_RDONLY);
	if (rc || rpmtsGetRdb(s->ts) == NULL) {
	    PyErr_SetString(pyrpmError, "rpmdb open failed");
	    goto exit;
	}
    }

    rpmdbIndexIterator ii = rpmdbIndexIteratorInit(rpmtsGetRdb(s->ts), tag);
    if (ii == NULL) {
        PyErr_SetString(PyExc_KeyError, "No index for this tag");
        return NULL;
    }
    mio = rpmii_Wrap(&rpmii_Type, ii, (PyObject*)s);

exit:
    return mio;
}


PyObject *rpmts_HdrFromFdno(rpmtsObject * s, PyObject *arg)
{
    PyObject *ho = NULL;
    rpmfdObject *fdo = NULL;
    Header h;
    rpmRC rpmrc;

    if (!PyArg_Parse(arg, "O&:HdrFromFdno", rpmfdFromPyObject, &fdo))
    	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    rpmrc = rpmReadPackageFile(s->ts, rpmfdGetFd(fdo), NULL, &h);
    Py_END_ALLOW_THREADS;
    Py_XDECREF(fdo);

    if (rpmrc == RPMRC_OK) {
	ho = hdr_Wrap(&hdr_Type, h);
    } else {
	Py_INCREF(Py_None);
	ho = Py_None;
    }
    return Py_BuildValue("(iN)", rpmrc, ho);
}

PyObject * rpmts_HdrCheck(rpmtsObject * s, PyObject *obj)
{
    PyObject * blob;
    char * msg = NULL;
    const void * uh;
    int uc;
    rpmRC rpmrc;

    if (!PyArg_Parse(obj, "S:HdrCheck", &blob))
    	return NULL;

    uh = PyBytes_AsString(blob);
    uc = PyBytes_Size(blob);

    Py_BEGIN_ALLOW_THREADS;
    rpmrc = headerCheck(s->ts, uh, uc, &msg);
    Py_END_ALLOW_THREADS;

    return Py_BuildValue("(iN)", rpmrc, utf8FromString(msg));
}

PyObject *rpmts_PgpPrtPkts(rpmtsObject * s, PyObject * args, PyObject * kwds)
{
    PyObject * blob;
    unsigned char * pkt;
    unsigned int pktlen;
    int rc;
    char * kwlist[] = {"octets", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S:PgpPrtPkts", kwlist, &blob))
    	return NULL;

    pkt = (unsigned char *)PyBytes_AsString(blob);
    pktlen = PyBytes_Size(blob);

    rc = pgpPrtPkts(pkt, pktlen, NULL, 1);

    return Py_BuildValue("i", rc);
}

PyObject *rpmts_PgpImportPubkey(rpmtsObject * s, PyObject * args, PyObject * kwds)
{
    PyObject * blob;
    unsigned char * pkt;
    unsigned int pktlen;
    int rc;
    char * kwlist[] = {"pubkey", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "S:PgpImportPubkey",
    	    kwlist, &blob))
	return NULL;

    pkt = (unsigned char *)PyBytes_AsString(blob);
    pktlen = PyBytes_Size(blob);

    rc = rpmtsImportPubkey(s->ts, pkt, pktlen);

    return Py_BuildValue("i", rc);
}

PyObject *rpmts_setKeyring(rpmtsObject *s, PyObject *arg)
{
    rpmKeyring keyring = NULL;
    if (arg == Py_None || rpmKeyringFromPyObject(arg, &keyring)) {
	return PyBool_FromLong(rpmtsSetKeyring(s->ts, keyring) == 0);
    } else {
	PyErr_SetString(PyExc_TypeError, "rpm.keyring or None expected");
	return NULL;
    }
}

PyObject *rpmts_getKeyring(rpmtsObject *s, PyObject *args, PyObject *kwds)
{
    rpmKeyring keyring = NULL;
    int autoload = 1;
    char * kwlist[] = { "autoload", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:getKeyring",
				     kwlist, &autoload))
	return NULL;

    keyring = rpmtsGetKeyring(s->ts, autoload);
    if (keyring) {
	return rpmKeyring_Wrap(&rpmKeyring_Type, keyring);
    } else {
	Py_RETURN_NONE;
    }
}

void *rpmtsCallback(const void * hd, const rpmCallbackType what,
		         const rpm_loff_t amount, const rpm_loff_t total,
	                 const void * pkgKey, rpmCallbackData data) {

    Header h = (Header) hd;
    struct rpmtsCallbackType_s * cbInfo = data;
    PyObject * pkgObj = (PyObject *) pkgKey;
    PyObject * args, * result;
    static FD_t fd;

    //printf ("*** Fulup rpmtsCallback tsid=%d count=%d \n", cbInfo->tsid, cbInfo->count++);
    if (cbInfo->cb == Py_None) return NULL;
    PyEval_RestoreThread(cbInfo->_save);

    /* Synthesize a python object for callback (if necessary). */
    if (pkgObj == NULL) {
        if (h) {
            pkgObj = utf8FromString(headerGetString(h, RPMTAG_NAME));
        } else {
            pkgObj = Py_None;
            Py_INCREF(pkgObj);
        }
    } else
    	Py_INCREF(pkgObj);
  
    args = Py_BuildValue("(iLLOO)", what, amount, total, pkgObj, cbInfo->data);
    result= PyEval_CallObject(cbInfo->cb, args);

    Py_DECREF(args);
    Py_DECREF(pkgObj);

    if (!result) 
        die(cbInfo->cb);

    if (what == RPMCALLBACK_INST_OPEN_FILE) {
        int fdno;

            if (!PyArg_Parse(result, "i", &fdno)) {
            die(cbInfo->cb);
        }
        Py_DECREF(result);
        cbInfo->_save = PyEval_SaveThread();

        fd = fdDup(fdno);
        fcntl(Fileno(fd), F_SETFD, FD_CLOEXEC);

        return fd;
    } else if (what == RPMCALLBACK_INST_CLOSE_FILE) {
	    Fclose (fd);
    }


    Py_DECREF(result);
    cbInfo->_save = PyEval_SaveThread();

    return NULL;
}

static int rpmts_SolveCallback(rpmts ts, rpmds ds, const void * data) {
    struct rpmtsCallbackType_s * cbInfo = (struct rpmtsCallbackType_s *) data;
    PyObject * args, * result;
    int res = 1;

    if (cbInfo->tso == NULL) return res;
    if (cbInfo->cb == Py_None) return res;

    PyEval_RestoreThread(cbInfo->_save);

    args = Py_BuildValue("(OiNNi)", cbInfo->tso,
		rpmdsTagN(ds), utf8FromString(rpmdsN(ds)),
		utf8FromString(rpmdsEVR(ds)), rpmdsFlags(ds));
    result = PyEval_CallObject(cbInfo->cb, args);
    Py_DECREF(args);

    if (!result) {
	die(cbInfo->cb);
    } else {
	if (PyLong_Check(result))
	    res = PyLong_AsLong(result);
	Py_DECREF(result);
    }

    cbInfo->_save = PyEval_SaveThread();
    return res;
}

PyObject *rpmts_Check(rpmtsObject * s, PyObject * args, PyObject * kwds) {
    struct rpmtsCallbackType_s cbInfo;
    int rc;
    char * kwlist[] = {"callback", NULL};

    memset(&cbInfo, 0, sizeof(cbInfo));
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:Check", kwlist,
	    &cbInfo.cb))
	return NULL;

    if (cbInfo.cb != NULL) {
	if (!PyCallable_Check(cbInfo.cb)) {
	    PyErr_SetString(PyExc_TypeError, "expected a callable");
	    return NULL;
	}
	rc = rpmtsSetSolveCallback(s->ts, rpmts_SolveCallback, (void *)&cbInfo);
    }

    cbInfo.tso = s;
    cbInfo._save = PyEval_SaveThread();
    rc = rpmtsCheck(s->ts);
    PyEval_RestoreThread(cbInfo._save);
    return PyBool_FromLong((rc == 0));
}

PyObject *rpmts_Order(rpmtsObject * s) {
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsOrder(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

PyObject *rpmts_Clean(rpmtsObject * s) {
    rpmtsClean(s->ts);

    Py_RETURN_NONE;
}

PyObject *rpmts_Clear(rpmtsObject * s) {
    rpmtsEmpty(s->ts);

    Py_RETURN_NONE;
}

PyObject *rpmts_Problems(rpmtsObject * s) {
    rpmps ps = rpmtsProblems(s->ts);
    PyObject *problems = rpmps_AsList(ps);
    rpmpsFree(ps);
    return problems;
}


PyObject * doLog(PyObject * self, PyObject * args, PyObject *kwds) {
    int code;
    const char *msg;
    char * kwlist[] = {"code", "msg", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "is", kwlist, &code, &msg))
	return NULL;

    rpmlog(code, "%s", msg);
    Py_RETURN_NONE;
}

PyObject * setLogFile (PyObject * self, PyObject *arg) {
    FILE *fp;
    int fdno = PyObject_AsFileDescriptor(arg);

    if (fdno >= 0) {
	/* XXX we dont know the mode here.. guessing append for now */
	fp = fdopen(fdno, "a");
	if (fp == NULL) {
	    PyErr_SetFromErrno(PyExc_IOError);
	    return NULL;
	}
    } else if (arg == Py_None) {
	fp = NULL;
    } else {
	PyErr_SetString(PyExc_TypeError, "file object or None expected");
	return NULL;
    }

    (void) rpmlogSetFile(fp);
    Py_RETURN_NONE;
}

PyObject *rpmts_OpenDB(rpmtsObject * s) {
    int dbmode;

    dbmode = rpmtsGetDBMode(s->ts);
    if (dbmode == -1)
	dbmode = O_RDONLY;

    return Py_BuildValue("i", rpmtsOpenDB(s->ts, dbmode));
}

PyObject *rpmts_CloseDB(rpmtsObject * s){
    int rc;

    rc = rpmtsCloseDB(s->ts);
    rpmtsSetDBMode(s->ts, -1);	/* XXX disable lazy opens */

    return Py_BuildValue("i", rc);
}

PyObject * rpmts_InitDB(rpmtsObject * s)
{
    int rc;

    rc = rpmtsInitDB(s->ts, O_RDONLY);
    if (rc == 0)
	rc = rpmtsCloseDB(s->ts);

    return Py_BuildValue("i", rc);
}

PyObject * rpmts_RebuildDB(rpmtsObject * s) {
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsRebuildDB(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

PyObject *rpmts_VerifyDB(rpmtsObject * s) {
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = rpmtsVerifyDB(s->ts);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

/* removed Fulup
PyObject * rpmts_dbCookie(rpmtsObject * s) {
    PyObject *ret = NULL;
    char *cookie = NULL;

    Py_BEGIN_ALLOW_THREADS
    cookie = rpmdbCookie(rpmtsGetRdb(s->ts));
    Py_END_ALLOW_THREADS

    ret = utf8FromString(cookie);
    free(cookie);
    return ret;
}
*/

PyObject *rpmts_iternext(rpmtsObject * s) {
    PyObject * result = NULL;
    rpmte te;

    /* Reset iterator on 1st entry. */
    if (s->tsi == NULL) {
	s->tsi = rpmtsiInit(s->ts);
	if (s->tsi == NULL)
	    return NULL;
    }

    te = rpmtsiNext(s->tsi, 0);
    if (te != NULL) {
	result = rpmte_Wrap(&rpmte_Type, te);
    } else {
	s->tsi = rpmtsiFree(s->tsi);
    }

    return result;
}

PyObject *rpmts_get_tid(rpmtsObject *s, void *closure) {
    return Py_BuildValue("i", rpmtsGetTid(s->ts));
}

PyObject *rpmts_get_color(rpmtsObject *s, void *closure) {
    return Py_BuildValue("i", rpmtsColor(s->ts));
}

PyObject *rpmts_get_prefcolor(rpmtsObject *s, void *closure) {
    return Py_BuildValue("i", rpmtsPrefColor(s->ts));
}

int rpmts_set_color(rpmtsObject *s, PyObject *value, void *closure) {
    rpm_color_t color;
    if (!PyArg_Parse(value, "i", &color)) return -1;

    /* TODO: validate the bits */
    rpmtsSetColor(s->ts, color);
    return 0;
}

int rpmts_set_prefcolor(rpmtsObject *s, PyObject *value, void *closure) {
    rpm_color_t color;
    if (!PyArg_Parse(value, "i", &color)) return -1;

    /* TODO: validate the bits */
    rpmtsSetPrefColor(s->ts, color);
    return 0;
}

PyObject *setVerbosity (PyObject * self, PyObject * arg) {
    int level;

    if (!PyArg_Parse(arg, "i", &level))
	return NULL;

    rpmSetVerbosity(level);

    Py_RETURN_NONE;
}


/*
 * Add rpm tag dictionaries to the module
 */
void addRpmTags(PyObject *module) {
    PyObject *pyval, *pyname, *dict = PyDict_New();
    rpmtd names = rpmtdNew();
    rpmTagGetNames(names, 1);
    const char *tagname, *shortname;
    rpmTagVal tagval;

    while ((tagname = rpmtdNextString(names))) {
	shortname = tagname + strlen("RPMTAG_");
	tagval = rpmTagGetValue(shortname);

	PyModule_AddIntConstant(module, tagname, tagval);
	pyval = PyLong_FromLong(tagval);
	pyname = utf8FromString(shortname);
	PyDict_SetItem(dict, pyval, pyname);
	Py_DECREF(pyval);
	Py_DECREF(pyname);
    }
    PyModule_AddObject(module, "tagnames", dict);
    rpmtdFree(names);
}

static int parseSignArgs(PyObject * args, PyObject *kwds,
			const char **path, struct rpmSignArgs *sargs)
{
    char * kwlist[] = { "path", "keyid", "hashalgo", NULL };

    memset(sargs, 0, sizeof(*sargs));
    return PyArg_ParseTupleAndKeywords(args, kwds, "s|si", kwlist,
				    path, &sargs->keyid, &sargs->hashalgo);
}

PyObject * addSign(PyObject * self, PyObject * args, PyObject *kwds) {
    const char *path = NULL;
    struct rpmSignArgs sargs;

    if (!parseSignArgs(args, kwds, &path, &sargs))
	return NULL;

    return PyBool_FromLong(rpmPkgSign(path, &sargs) == 0);
}

PyObject * delSign(PyObject * self, PyObject * args, PyObject *kwds) {
    const char *path = NULL;
    struct rpmSignArgs sargs;

    if (!parseSignArgs(args, kwds, &path, &sargs))
	return NULL;

    return PyBool_FromLong(rpmPkgDelSign(path, &sargs) == 0);
}


PyObject *rpmts_get_rootDir(rpmtsObject *s, void *closure) {
    return utf8FromString(rpmtsRootDir(s->ts));
}


PyObject *rpmts_get_flags(rpmtsObject *s, void *closure) {
    return Py_BuildValue("i", rpmtsFlags(s->ts));
}

int rpmts_set_flags(rpmtsObject *s, PyObject *value, void *closure) {
    rpmtransFlags flags;
    if (!PyArg_Parse(value, "i", &flags)) return -1;

    /* TODO: validate the bits */
    rpmtsSetFlags(s->ts, flags);
    return 0;
}

PyObject *rpmts_get_vsflags(rpmtsObject *s, void *closure) {
    return Py_BuildValue("i", rpmtsVSFlags(s->ts));
}

int rpmts_set_vsflags(rpmtsObject *s, PyObject *value, void *closure) {
    rpmVSFlags flags;
    if (!PyArg_Parse(value, "i", &flags)) return -1;

    /* TODO: validate the bits */
    rpmtsSetVSFlags(s->ts, flags);
    return 0;
}
