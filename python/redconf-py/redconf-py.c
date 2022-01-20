/*
* Copyright (C) 2022 "IoT.bzh"
* Clément Bénier <clement.benier@iot.bzh>
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
*/

#define PY_SSIZE_T_CLEAN  /* Make "s#" use Py_ssize_t rather than int. */
#include <Python.h>

#include "redconf-merge.h"

static PyObject *mergeconfig(PyObject *self, PyObject *args) {
    const char *mergedconfig;
    const char *redpath  = NULL;
    size_t len;
    PyObject *o = NULL;

    if(!PyArg_ParseTuple(args, "s", &redpath)) {
        return NULL;
    }

    mergedconfig = getMergeConfig(redpath, &len, 0);

    if(mergedconfig) {
        o = PyBytes_FromStringAndSize(mergedconfig, len);
        free((char *)mergedconfig);
    }

    return o;
}

static PyMethodDef redconf_methods[] = {
    {"mergeconfig", mergeconfig, METH_VARARGS, "Python interface for mergeconfig redpak"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef redconf_module = {
    PyModuleDef_HEAD_INIT,
    "redconf",
    "Python interface for redconf",
    -1,
    redconf_methods
};

PyMODINIT_FUNC PyInit_redconf(void) {
    return PyModule_Create(&redconf_module);
}