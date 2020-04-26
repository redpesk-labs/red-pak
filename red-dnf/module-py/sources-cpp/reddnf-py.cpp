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
* Documentation: https://gist.github.com/physacco/2e1b52415f3a964ad2a542a99bebed8f
*/

#include <Python.h>
#include <unistd.h>
#include <libdnf/dnf-sack.h>
#include <solv/repo.h>

#include "reddnf-py.h"

// Missing public but unsigned function
DnfSack *sackFromPyObject(PyObject *o);
PyObject *op_error2exc(const GError *error);
HyRepo  repoFromPyObject(PyObject *o);
PyObject *repoToPyObject(HyRepo);

static PyObject * repo_set_rootdir(PyObject *self, PyObject *args) {    
    const char *rootdir;
    PyObject *sackPy=NULL;

    if (!PyArg_ParseTuple(args, "Os", &sackPy, &rootdir)) {
        PyErr_SetString(PyExc_SystemError, "usage: _repo_set_rootdir (sack_obj, 'rootdir_string')");
        return NULL;
    }

    // retreive sack from python object
    DnfSack *sack = sackFromPyObject(sackPy);
    assert (sack);

    dnf_sack_set_rootdir(sack, rootdir);
    Py_RETURN_NONE; 
}

static PyObject * repo_create_empty (PyObject *self, PyObject *args) {
    PyObject *sackPy=NULL; 
    const char *reponame;

    if (!PyArg_ParseTuple(args, "Os", &sackPy, &reponame)) {
        PyErr_SetString(PyExc_SystemError, "usage: _repo_create_empty (sack, 'reponame')");
        return NULL;
    }

    // retreive pool from sack object
    DnfSack *sack = sackFromPyObject(sackPy);
    Pool *pool= dnf_sack_get_pool(sack);

    Repo *repo = repo_create(pool, reponame);
    pool_set_installed(pool, repo);

    Py_RETURN_NONE;
}


static PyObject * repo_load_rpmdb (PyObject *self, PyObject *args) {    
    PyObject *sackPy=NULL;
    PyObject *hrepoPy=NULL;
    const char *rootdir;
    const char *reponame;
    gboolean error;
    g_autoptr(GError) gerror = NULL;

    if (!PyArg_ParseTuple(args, "Oss", &sackPy, &reponame, &rootdir)) {
        PyErr_SetString(PyExc_SystemError, "usage: _repo_load_rpmdb (sack, 'reponame', 'rootdir_string')");
        return NULL;
    }

    // extract sack and repo from python object
    DnfSack *sack = sackFromPyObject(sackPy);
    //HyRepo hrepo = repoFromPyObject(hrepoPy);

    dnf_sack_set_rootdir(sack, rootdir);
    error = dnf_sack_add_rpmdb_repo (sack, reponame, &gerror);
    if (!error)
        return op_error2exc(gerror);

    Py_RETURN_NONE; 
}

static PyMethodDef redpakMethods[] = {
    {"_repo_create_empty", (PyCFunction)repo_create_empty, METH_VARARGS | METH_KEYWORDS, "Load Installed RPM forcing reponame and rootdir"},
    {"_repo_load_rpmdb", (PyCFunction)repo_load_rpmdb, METH_VARARGS | METH_KEYWORDS, "Load Installed RPM forcing reponame and rootdir"},
    {"_repo_set_rootdir", (PyCFunction)repo_set_rootdir, METH_VARARGS | METH_KEYWORDS, "Change DNF/RPM rootdir dynamically"},
    {NULL}  /* sentinel */
};

static struct PyModuleDef redpakDefinition = { 
    PyModuleDef_HEAD_INIT,
    "libredpak",
    "Python 'redpak' plugin native method extention.",
    -1, 
    redpakMethods
};

// Init redpak native module
PyMODINIT_FUNC PYTHON_MODULE_ENTRY {
    PyObject *module;

    Py_Initialize();
    return PyModule_Create(&redpakDefinition);
}