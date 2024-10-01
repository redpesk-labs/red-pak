/*
* Copyright (C) 2020 "IoT.bzh"
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

#ifndef _RED_CONF_EXPAND_INCLUDE_
#define _RED_CONF_EXPAND_INCLUDE_

#include "redconf-node.h"

/**
* Returns a fresh allocated string being the expansion of @ref key
* in the context of @ref node.
*
* @param node     the node to use for contextual expansion
* @param key      the default key to expand (without $)
*
* @return return the default expansion of @ref key or
*         return NULL when (1) allocation failed or (2) expansion is too large
*         or (3) expansion reached errors.
*/
extern char *RedGetDefaultExpand(const redNodeT *node, const char* key);

/**
* This function expands in the buffer @ref outputS, of @ref maxlen length
* and indexed by *@ref idxOut, the  expansion of @ref inputS
* in the context of @ref node.
*
* On termination, *@ref idxOut is updated and a trailing zero is appended.
*
* Expansion of commands $(...) are NOT allowed by this function.
*
* When @ref prefix isn't NULL, it is prepended to the expansion.
* When @ref suffix isn't NULL, it is appended to the expansion.
* But if expansions lead to an empty string, nothing is neither
* prepended nor appended to that empty string.
*
* @param node     the node to use for contextual expansion
* @param outputS  output string buffer receiving expansion
* @param idxOut   pointer to the integer index in @ref outputS
* @param maxlen   length of the output string buffer
* @param inputS   the string to expand (cannot be NULL)
* @param prefix   if not NULL a prepended string
* @param suffix   if not NULL an appended string
*
* @return 0 in case of success or 1 in case of too large expansion.
*/
extern int RedConfAppendEnvKey(const redNodeT *node,
                                char *outputS, int *idxOut, int maxlen,
                                const char *inputS, const char* prefix, const char *suffix);

/**
* Returns a fresh allocated string being the expansion of the input
* string @ref inputS in the context of @ref node.
*
* Expansion of commands $(...) are allowed by this function.
*
* @param node     the node to use for contextual expansion
* @param inputS   the string to expand (cannot be NULL)
*
* @return return the default expansion of @ref key or
*         return NULL when (1) allocation failed or (2) expansion is too large
*         or (3) expansion reached errors.
*/
extern char *RedNodeStringExpand(const redNodeT *node, const char* inputS);

/**
* Returns a fresh allocated string being the copy of the input
* string @ref input.
*
* When @ref expand is not zero, the variables are expanded using the
* function @ref RedNodeStringExpand and the context given by @ref node
*.
*
* @param node   the node to use for contextual expansion
* @param input  the string to copy and/or expand (can be NULL)
* @param expand boolean telling if expansion is required or not
*
* @return return the expanded (or not) copy of @ref input or
*         return NULL when (1) input is NULL or (2) allocation failed
*         or (3) expansion is too large or (4) expansion reached errors.
*/
extern char *expandAlloc(const redNodeT *node, const char *input, int expand);

/**
* This function expands in the buffer @ref outputS, of @ref maxlen length
* and indexed by *@ref idxOut, the path expansion of @ref inputS
* in the context of @ref node.
*
* On termination, *@ref idxOut is updated and a trailing zero is appended.
*
* @param node     the node to use for contextual expansion
* @param outputS  output string buffer receiving expansion
* @param idxOut   pointer to the integer index in @ref outputS
* @param maxlen   length of the output string buffer
* @param inputS   the string to expand (cannot be NULL)
*
* @return 0 in case of success or 1 in case of too large expansion.
*/
extern int RedConfAppendPath(const redNodeT *node,
                             char *outputS, int *idxOut, int maxlen,
                             const char *inputS);

#endif
