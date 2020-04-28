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

int redts_init(rpmtsObject *s, PyObject *args, PyObject *kwds);
PyObject *redts_AddErase(rpmtsObject *s, PyObject * args);
PyObject *redts_AddReinstall(rpmtsObject * s, PyObject * args);
PyObject * redts_AddInstall(rpmtsObject * s, PyObject * args);
PyObject *redts_Run(rpmtsObject *s, PyObject *args, PyObject *kwds);
