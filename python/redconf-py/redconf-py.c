/*
* Copyright (C) 2022-2025 IoT.bzh Company
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
#include <stdio.h>
#include <fcntl.h>

#include "redconf-merge.h"

static PyObject *RedConfError = NULL;

//{
//    int stdout_dupfd;
//    FILE *temp_out;
//
//    /* duplicate stdout */
//    stdout_dupfd = _dup(1);
//
//    temp_out = fopen("file.txt", "w");
//
//    /* replace stdout with our output fd */
//    _dup2(_fileno(temp_out), 1);
//    /* output something... */
//    printf("Woot!\n");
//    /* flush output so it goes to our file */
//    fflush(stdout);
//    fclose(temp_out);
//    /* Now restore stdout */
//    _dup2(stdout_dupfd, 1);
//    _close(stdout_dupfd);
//}

static FILE *savedstderr = NULL;

static FILE *catchstderr() {

    // save right stderr
    savedstderr = (FILE *)stderr;

    //redirect stderr
    FILE *fstderr = tmpfile();
    stderr = fstderr;
    return fstderr;
}

static void resetstderr(FILE *fstderr, int error) {
    if(!fstderr) return;

    if (!error)
        goto Exit;

    char *buf;
    size_t size, rsz;

    fflush(fstderr);
    //get size of fstderr and alloc buffer
    size = ftell(fstderr);
    buf = malloc(size + 1);

    //reset cursor at the beginning of the FILE
    fseek(fstderr, 0, SEEK_SET);
    rsz = fread(buf, 1, size, fstderr);
    buf[rsz] = 0;

    //set stderr to python exception
    PyErr_SetString(RedConfError, buf);
    free(buf);

Exit:
    //restore stderr
    stderr = savedstderr;
    fclose(fstderr);
}

static PyObject *mergeconfig(PyObject *self, PyObject *args) {
    const char *mergedconfig;
    const char *redpath  = NULL;
    size_t len;
    PyObject *o = NULL;
    FILE *fstderr = NULL;
    int error = 0;

    (void)self;

    if(!PyArg_ParseTuple(args, "s", &redpath)) {
        goto OnExit;
    }

    fstderr = catchstderr();
    mergedconfig = getMergeConfig(redpath, &len, 0);

    if(!mergedconfig) {
        error = 1;
        goto OnExit;
    }

    o = PyBytes_FromStringAndSize(mergedconfig, len);
    free((char *)mergedconfig);

OnExit:
    resetstderr(fstderr, error);
    return o;
}

static PyObject *config(PyObject *self, PyObject *args) {
    const char *config;
    const char *redpath  = NULL;
    size_t len;
    PyObject *o = NULL;
    FILE *fstderr;
    int error = 0;

    (void)self;

    if(!PyArg_ParseTuple(args, "s", &redpath)) {
        return NULL;
    }

    fstderr = catchstderr();
    config = getConfig(redpath, &len);

    if(!config) {
        error = 1;
        goto OnExit;
    }

    o = PyBytes_FromStringAndSize(config, len);
    free((char *)config);

OnExit:
    resetstderr(fstderr, error);
    return o;
}

static PyMethodDef redconf_methods[] = {
    {"mergeconfig", mergeconfig, METH_VARARGS, "Python interface for mergeconfig redpak"},
    {"config", config, METH_VARARGS, "Python interface for config redpak"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef redconf_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "redconf",
    .m_doc = "Python interface for redconf",
    .m_size = -1,
    .m_methods = redconf_methods,
    .m_slots = 0,
    .m_traverse = 0,
    .m_clear = 0,
    .m_free = 0
};

PyMODINIT_FUNC
PyInit_redconf(void)
{
    PyObject *m;

    m = PyModule_Create(&redconf_module);
    if (m == NULL)
        return NULL;

    RedConfError = PyErr_NewException("redconf.error", NULL, NULL);
    Py_XINCREF(RedConfError);
    if (PyModule_AddObject(m, "error", RedConfError) < 0) {
        Py_XDECREF(RedConfError);
        Py_CLEAR(RedConfError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}