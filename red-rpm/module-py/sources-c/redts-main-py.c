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
    Implement RedTransactionCore class and gue python with C libRPM 
    Freely inspired from original librpm rpmts-py.c and rpmmodule.c

    Everything related to initial Python object used by RedRPM start
    from this file.
*/

#define _GNU_SOURCE 1
#include <stdio.h>

#include <fcntl.h>
#include <rpm/rpmspec.h>
#include <rpm/rpmbuild.h>

#include "redts-main-py.h"
#include "redts-import-py.h"
#include "redts-doc.inc"

#include "redconf.h"
#include "redts-glue-py.h"

// Helpers to make code look cleaner
#define MOD_REGISTER_ENUM(VAL) PyModule_AddIntConstant(module,#VAL,(long)VAL)

// Change Original LibRpm object name and register corresponding types
#define MOD_REGISTER_TYPE(TYPE_NAME,TYPE_ADDR) \
    asprintf ((char**)&redname, "redrpm.%s", TYPE_NAME); \
    TYPE_ADDR.tp_name = redname; \
    Py_INCREF(& TYPE_ADDR); \
    PyModule_AddObject(module,TYPE_NAME,(PyObject*)& TYPE_ADDR) \

// global Module Py Error
PyObject* pyrpmError;
static PyTypeObject rpmts_Type;


static PyObject * rpmts_new(PyTypeObject * subtype, PyObject *args, PyObject *kwds) {
    static int count=0;
    const char *rootdir;
    rpmtsObject * s = (rpmtsObject *)subtype->tp_alloc(subtype, 0);
    if (s == NULL) return NULL;

    //PyArg_ParseTuple(args, "s", &rootdir);
    //printf ("==== Fulup rpmts_new tsid=%d rootdir=%s\n",count, rootdir);

    s->ts = rpmtsCreate();
    s->tsid= count++;
    s->tsi = NULL;
    s->keyList = PyList_New(0);
    return (PyObject *) s;
}

static void rpmts_dealloc(rpmtsObject *s) {
    //printf ("**** Fulup rpmts_dealloc tsid=%d\n",s->tsid);
    s->ts = rpmtsFree(s->ts);
    Py_XDECREF(s->keyList);
    Py_TYPE(s)->tp_free((PyObject *)s);
}

// Add Complementary Elements to Module
int ModAddElements(PyObject *module) {

    // Failling to read RPM conf and Crypto is fatal
    if (rpmReadConfigFiles(NULL, NULL) < 0) goto OnErrorExit;

    // register module global error object
    PyObject *dict = PyModule_GetDict(module);
    pyrpmError = PyErr_NewException("_redrpm.error", NULL, NULL);
    if (!pyrpmError )  goto OnErrorExit;
	PyDict_SetItemString(dict, "error", pyrpmError);

    // prefix every type with "redrpm." (this hack allow to change from 'rpm' to 'redrpm' while keep unchanged 'rpmlib-ori' code)
    const char *redname; // warning use by MOD_REGISTER_TYPE macro
    MOD_REGISTER_TYPE ("hdr", hdr_Type);
    MOD_REGISTER_TYPE ("ds", rpmds_Type);
    MOD_REGISTER_TYPE ("fd", rpmfd_Type);
    MOD_REGISTER_TYPE ("fi", rpmfi_Type);
    MOD_REGISTER_TYPE ("file", rpmfile_Type);
    MOD_REGISTER_TYPE ("files", rpmfiles_Type);
    MOD_REGISTER_TYPE ("keyring", rpmKeyring_Type);
    MOD_REGISTER_TYPE ("mi", rpmii_Type);
    MOD_REGISTER_TYPE ("ii", rpmii_Type);
    MOD_REGISTER_TYPE ("prob", rpmProblem_Type);
    MOD_REGISTER_TYPE ("pubkey", rpmPubkey_Type);
    MOD_REGISTER_TYPE ("strpool", rpmstrPool_Type);
    MOD_REGISTER_TYPE ("te", rpmte_Type);
    MOD_REGISTER_TYPE ("ts", rpmts_Type);
 
    // ADD contante to rpmPy Object ex: rpm.RPMTAG_PROVIDENAME
    addRpmTags(module);
    PyModule_AddStringConstant(module, "__version__", RPMVERSION);
    PyModule_AddObject(module, "header_magic", PyBytes_FromStringAndSize((const char *)rpm_header_magic, 8));
   
    // Finally add every needed enumerations value 
    #include "redts-enum.inc"

    return 0;

OnErrorExit:
    return 1;    
}

// used by DNF !!! only provide a setter until we have rpmfd wrappings 
static PyGetSetDef rpmts_getseters[] = {
    
	{"_flags",	  (getter)rpmts_get_flags, (setter)rpmts_set_flags, NULL},
	{"_vsflags",  (getter)rpmts_get_vsflags, (setter)rpmts_set_vsflags, NULL},
  	{"_color",	(getter)rpmts_get_color, (setter)rpmts_set_color, NULL},
	{"_prefcolor",	(getter)rpmts_get_prefcolor, (setter)rpmts_set_prefcolor, NULL},
   	{"rootDir",	(getter)rpmts_get_rootDir, NULL, "read only, directory rpm treats as root of the file system." },
    {"tid",		(getter)rpmts_get_tid, NULL, "Get current transaction id [transaction time stamp]."},
	{ NULL }
};

static int InitModTypes(void) {
    if (PyType_Ready(&hdr_Type) < 0) return 0;
    if (PyType_Ready(&rpmds_Type) < 0) return 0;
    if (PyType_Ready(&rpmfd_Type) < 0) return 0;
    if (PyType_Ready(&rpmfi_Type) < 0) return 0;
    if (PyType_Ready(&rpmfile_Type) < 0) return 0;
    if (PyType_Ready(&rpmfiles_Type) < 0) return 0;
    if (PyType_Ready(&rpmKeyring_Type) < 0) return 0;
    if (PyType_Ready(&rpmmi_Type) < 0) return 0;
    if (PyType_Ready(&rpmii_Type) < 0) return 0;
    if (PyType_Ready(&rpmProblem_Type) < 0) return 0;
    if (PyType_Ready(&rpmPubkey_Type) < 0) return 0;
    if (PyType_Ready(&rpmstrPool_Type) < 0) return 0;
    if (PyType_Ready(&rpmte_Type) < 0) return 0;
    if (PyType_Ready(&rpmts_Type) < 0) return 0;
    return 0;
}

// Transaction object methods with minimal help text
static struct PyMethodDef rpmts_methods[] = {
 {"addInstall"   ,	(PyCFunction) redts_AddInstall,	METH_VARARGS, "Install a package: ts.addInstall(hdr, data, mode)."},
 {"addReinstall" ,	(PyCFunction) redts_AddReinstall, METH_VARARGS, "Update a package: ts.addReinstall(hdr, data)."},
 {"addErase"     ,	(PyCFunction) redts_AddErase, METH_VARARGS|METH_KEYWORDS, "Remove an intalled package: addErase(name)."},

 {"check"        ,	(PyCFunction) rpmts_Check, METH_VARARGS|METH_KEYWORDS, "Perform depencies check on transaction: ts.check()."},
 {"order"        ,	(PyCFunction) rpmts_Order, METH_NOARGS, "Sort added element in transaction: ts.order()." },
 {"problems"     ,	(PyCFunction) rpmts_Problems, METH_NOARGS, "Return current problem set: ts.problems()."},
 {"clean"        ,	(PyCFunction) rpmts_Clean, METH_NOARGS,  "Free memory: ts.clean() [generally not used]." },
 {"clear"        ,	(PyCFunction) rpmts_Clear, METH_NOARGS, "Remove all elements from transaction:  ts.clear()."},
 {"run"          ,	(PyCFunction) redts_Run, METH_VARARGS|METH_KEYWORDS, "Run & return transaction unsolved issues: ts.run(callback, data)."},


 {"openDB"       ,	(PyCFunction) rpmts_OpenDB, METH_NOARGS, "Open the default transaction rpmdb ts.openDB()  [generally not used]." }, 
 {"closeDB"      ,	(PyCFunction) rpmts_CloseDB, METH_NOARGS, "Close the default transaction rpmdb: ts.closeDB() [generally not used]."},
 {"initDB"       ,	(PyCFunction) rpmts_InitDB, METH_NOARGS, "Initialize default transaction rpmdb: ts.initDB()."}, 
 {"rebuildDB"    ,	(PyCFunction) rpmts_RebuildDB, METH_NOARGS, "Rebuild the default transaction rpmdb: ts.rebuildDB()."},
 {"verifyDB"     ,	(PyCFunction) rpmts_VerifyDB, METH_NOARGS, "Verify the default transaction rpmdb: ts.verifyDB()."},
 {"dbIndex"      ,  (PyCFunction) rpmts_index, METH_VARARGS|METH_KEYWORDS, "Create a key iterator: ts.dbIndex(TagN)" },
 // {"dbCookie"     ,	(PyCFunction) rpmts_dbCookie, METH_NOARGS, "Return a cookie to check if DB changed: dbCookie -> cookie" },
 {"dbMatch"      ,	(PyCFunction) rpmts_Match, METH_VARARGS|METH_KEYWORDS, "Create a match iterator: ts.dbMatch([TagN, [key]])" },

 {"hdrFromFdno"  ,  (PyCFunction) rpmts_HdrFromFdno,METH_O, "Read a package header from a file descriptor: ts.hdrFromFdno(fdno)."},
 {"hdrCheck"     ,	(PyCFunction) rpmts_HdrCheck, METH_O, "Check header consistency: ts.hdrCheck(hdrblob)."},


 {"pgpPrtPkts"   ,  (PyCFunction) rpmts_PgpPrtPkts, METH_VARARGS|METH_KEYWORDS, "Print/parse a OpenPGP packet(s):  pgpPrtPkts(octets)." },
 {"pgpImportPubkey",(PyCFunction) rpmts_PgpImportPubkey, METH_VARARGS|METH_KEYWORDS, "mport public key packet: pgpImportPubkey(pubkey)." },

 { "setVerbosity", (PyCFunction) setVerbosity, METH_O,  "Set log level: setVerbosity(level)" },

 {"getKeyring"   ,  (PyCFunction) rpmts_getKeyring, METH_VARARGS|METH_KEYWORDS, " -- Return key ring object." },
 {"setKeyring"   ,  (PyCFunction) rpmts_setKeyring, METH_O, "Set key ring used for checking signatures: ts.setKeyring(keyring)"},

 {NULL}		/* sentinel */
};

// Module subset methods from original rpmlib module (not to be confused with transaction object methods)
static PyMethodDef rpmModuleMethods[] = {
 { "log", (PyCFunction) doLog, METH_VARARGS|METH_KEYWORDS, "log(level, msg) level must be one of the RPMLOG_* constants."},
 { "setLogFile", (PyCFunction) setLogFile, METH_O, "setLogFile(file) -- set file to write log messages to or None." },
 { "setVerbosity", (PyCFunction) setVerbosity, METH_O, "setVerbosity(level) -- Set log level. See RPMLOG_* constants." },
 { "addSign", (PyCFunction) addSign, METH_VARARGS|METH_KEYWORDS, NULL },
 { "delSign", (PyCFunction) delSign, METH_VARARGS|METH_KEYWORDS, NULL },
 { "addMacro"    , (PyCFunction) rpmmacro_AddMacro, METH_VARARGS|METH_KEYWORDS, "rpmPushMacro(macro, value)"},
 { "delMacro"    , (PyCFunction) rpmmacro_DelMacro, METH_VARARGS|METH_KEYWORDS,"rpmPopMacro(macro)"},
 { "expandMacro" , (PyCFunction) rpmmacro_ExpandMacro, METH_VARARGS|METH_KEYWORDS, "expands a string containing macros: expandMacro(string, numeric=False)"},

    {NULL} /* sentinel */
};

// transaction object
static PyTypeObject rpmts_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	.tp_name      = "_redrpm.ts",	
	.tp_basicsize = sizeof(rpmtsObject),
    .tp_itemsize  = 0,
	.tp_dealloc   = (destructor) rpmts_dealloc, 
	.tp_getattro  = PyObject_GenericGetAttr,
	.tp_setattro  = PyObject_GenericSetAttr,
	.tp_flags     = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	
	.tp_doc       = rpmts_doc,
	.tp_iter      = PyObject_SelfIter,
	.tp_iternext  = (iternextfunc) rpmts_iternext,
	.tp_methods   = rpmts_methods,
	.tp_getset    = rpmts_getseters,
	.tp_init      = (initproc)redts_init,
	.tp_new       = (newfunc) rpmts_new,
};

static int rpmModuleTraverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(pyrpmError);
    return 0;
}

static int rpmModuleClear(PyObject *m) {
    Py_CLEAR(pyrpmError);
    return 0;
}

static struct PyModuleDef RedRpmModDef = {
    .m_base   = PyModuleDef_HEAD_INIT,
    .m_name   = "_redrpm",            
    .m_doc    = "redrpm expose redrpm and librpm C API in a DNF compatible manner", 
    .m_size   = 0,
    .m_methods= rpmModuleMethods,
    .m_slots  = NULL,
    .m_traverse=rpmModuleTraverse,
    .m_clear  = rpmModuleClear,
    .m_free   = NULL,
};


// Create Module Object dependencies, then load and initialise module
PyMODINIT_FUNC PYTHON_MODULE_ENTRY {

    // Register the python custom types used by this module
    if (InitModTypes()) return NULL;

    // Add module within Python interpreter
    PyObject *module = PyModule_Create(&RedRpmModDef);

    // Complet module with its associated elements
    if (module) ModAddElements(module);
    return module;
}