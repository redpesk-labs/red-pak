/*
* Copyright (C) 2022-2025 IoT.bzh Company
* Author: José Bollo <jose.bollo@iot.bzh>
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

#ifndef _RED_CONF_MIX_INCLUDE_
#define _RED_CONF_MIX_INCLUDE_

#include "redconf-node.h"



/****************************************************************************
* merge of path like features
****************************************************************************/

/**
 * callback receiving merged paths
 *
 * @param closure a closure
 * @param path    merged path zero terminated
 * @param length  length (position of terminating zero)
 *
 * @return a status that will be returned
 */
typedef int (*mix_path_cb)(void *closure, const char *path, size_t length);

/**
 * callback for getting a path to expand and merge
 *
 * @param closure a closure
 * @param config  a config item
 *
 * @return the string to expand and merge or NULL
 */
typedef const char *(*mix_get_path_cb)(void *closure, const redConfigT *config);

/**
 * expand and merge path like features
 *
 * @param node     the node
 * @param getpath  the function for getting path like items
 * @param callback the function receiving the merged paths
 * @param closure  a closure
 *
 * @return a status either the status returned by callback or the negative
 * error code returned by function RedConfAppendExpandedPath
 */
extern int mixPaths(const redNodeT *node, mix_get_path_cb getpath, mix_path_cb callback, void *closure);

/****************************************************************************
* merge of environment variables
****************************************************************************/

/**
* callback receiving the merged variables
* received indexes are upward counting from 0 to count - 1
*
* @param closure  a closure
* @param var      the variable
* @param node     the node of the variable (for expansion)
* @param index    index of variable
* @param count    count of variables
*
* @return 0 for continuing processing or an other value
* that will be return ed at end
*/
typedef int (*mix_var_cb)(void *closure, const redConfVarT *var, const redNodeT *node, unsigned index, unsigned count);

/**
* scan the node, merge its variables and at the end invoke the callback
* with the merged variables
*
* @param node      the rednode
* @param callback  the callback
* @param closure   closure for the callback
*
* @return -1 on error or the last non null value returned by callback
*/
extern int mixVariables(const redNodeT *node, mix_var_cb callback, void *closure);

/****************************************************************************
* merge of capabilities
****************************************************************************/

/**
* callback receiving the merged capabilities
* received indexes are upward counting from 0 to count - 1
*
* @param closure    a closure
* @param capability the name of the capability
* @param value      the merged value
* @param index      index of the capability
* @param count      count of capabilities
*
* @return 0 for continuing processing or an other value
* that will be return ed at end
*/
typedef int (*mix_capa_cb)(void *closure, const char *capability, int value, unsigned index, unsigned count);

/**
* scan the node, merge its capabilities and at the end invoke the callback
* with the merged capabilities
*
* @param node      the rednode
* @param callback  the callback
* @param closure   closure for the callback
*
* @return -1 on error or the last non null value returned by callback
*/
extern int mixCapabilities(const redNodeT *node, mix_capa_cb callback, void *closure);

/****************************************************************************
* merge of exports
****************************************************************************/

/**
* callback receiving the merged exports
* received indexes are upward counting from 0 to count - 1
*
* @param closure  a closure
* @param exp      the export
* @param node     the node of the export (for expansion)
* @param index    index of export
* @param count    count of exports
*
* @return 0 for continuing processing or an other value
* that will be return ed at end
*/
typedef int (*mix_exp_cb)(void *closure, const redConfExportPathT *exp, const redNodeT *node, unsigned index, unsigned count);

/**
* scan the node, merge its exports and at the end invoke the callback
* with the merged exports
*
* @param node      the rednode
* @param callback  the callback
* @param closure   closure for the callback
*
* @return -1 on error or the last non null value returned by callback
*/
extern int mixExports(const redNodeT *node, mix_exp_cb callback, void *closure);

/****************************************************************************
* merge of sharings
****************************************************************************/

/**
* callback receiving the merged sharings
*
* @param closure  a closure
* @param shares   the sharing
*/
typedef int (*mix_share_cb)(void *closure, const redConfShareT *shares);

/**
* scan the node, merge its sharings and at the end invoke the callback
* with the merged sharings
*
* @param node      the rednode
* @param callback  the callback
* @param closure   closure for the callback
*
* @return -1 on error or the result of the callback
*/
extern int mixShares(const redNodeT *node, mix_share_cb callback, void *closure);

/****************************************************************************
* merge of conftag
****************************************************************************/

typedef int (*mix_conftag_cb)(void *closure, const redConfTagT *conftag);
extern int mixConfTags(const redNodeT *node, mix_conftag_cb callback, void *closure);

/****************************************************************************
* merge of early conf
****************************************************************************/

typedef int (*mix_early_conf_cb)(void *closure, const early_conf_t *conf, const redNodeT *node);

extern int mixEarlyConf(const redNodeT *node, mix_early_conf_cb callback, void *closure);

#endif
