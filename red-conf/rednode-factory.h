/*
 Copyright (C) 2021-2025 IoT.bzh Company

 Author: José Bollo <jose.bollo@iot.bzh>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef _REDNODE_FACTORY_INCLUDED_
#define _REDNODE_FACTORY_INCLUDED_

#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

/* max path length */
#include "redconf-defaults.h"
#define REDNODE_FACTORY_PATH_LEN  RED_MAXPATHLEN

/** error code */
enum rednode_factory_error_e
{
/*  0 */    RednodeFactory_OK,
/*  1 */    RednodeFactory_Error_Cleared,
/*  2 */    RednodeFactory_Error_Allocation,
/*  3 */    RednodeFactory_Error_Root_Too_Long,
/*  4 */    RednodeFactory_Error_Root_Not_Absolute,
/*  5 */    RednodeFactory_Error_Root_Not_Exist,
/*  6 */    RednodeFactory_Error_Node_Too_Long,
/*  7 */    RednodeFactory_Error_Default_Alias_Empty,
/*  8 */    RednodeFactory_Error_MkDir,
/*  9 */    RednodeFactory_Error_FmtDate,
/* 10 */    RednodeFactory_Error_TemplateDir_Too_Long,
/* 11 */    RednodeFactory_Error_Template_Too_Long,
/* 12 */    RednodeFactory_Error_No_Template,
/* 13 */    RednodeFactory_Error_Loading_Template,
/* 14 */    RednodeFactory_Error_Config_Exist,
/* 15 */    RednodeFactory_Error_Storing_Config,
/* 16 */    RednodeFactory_Error_Path_Too_Long,
/* 17 */    RednodeFactory_Error_Storing_Status,
/* 18 */    RednodeFactory_Error_At_Root,
/* 19 */    RednodeFactory_Error_Config_Not_Exist,
/* 20 */    RednodeFactory_Error_Invalid_Alias,
};

/** creation modes */
enum rednode_factory_mode_e
{
    /** create a node (default leaf) */
    RednodeFactory_Mode_Default,
    /** create a node (default leaf) and if absent a parent root node */
    RednodeFactory_Mode_Legacy,
    /** create a node (default full) */
    RednodeFactory_Mode_Legacy_NoSys
};

/* parameters of node creation */
struct rednode_factory_param_s
{
    /* alias name (NULL for defaulting to node directory name) */
    const char *alias;
    /* normal config name or path to a normal configuration file (NULL for default name) */
    const char *normal;
    /* admin config name or path to an admin configuration file (NULL for default name) */
    const char *admin;
    /* template directory to be used (NULL for default name) */
    const char *templatedir;
};

/* structure for creating red node */
struct rednode_factory_s
{
    /* length of rootdir in path (including enforced trailing slash) */
    size_t root_length;
    /* length of nodedir in path (including enforced trailing slash and root dir) */
    size_t node_length;
    /* the path */
    char path[REDNODE_FACTORY_PATH_LEN];
};

typedef struct rednode_factory_s       rednode_factory_t;
typedef struct rednode_factory_param_s rednode_factory_param_t;
typedef enum   rednode_factory_error_e rednode_factory_error_t;
typedef enum   rednode_factory_mode_e  rednode_factory_mode_t;

/** Default model name for full config (no-system) */
extern const char rednode_factory_deftmpl_full[];
/** Default model name for root config (system node) */
extern const char rednode_factory_deftmpl_root[];
/** Default model name for leaf config (leaf node) */
extern const char rednode_factory_deftmpl_leaf[];

/**
 * Get text of the error report
 *
 * @param error error code
 *
 * @return the string corresponding to the given error code
 */
extern const char *rednode_factory_error_text(rednode_factory_error_t error);

/**
 * Clears the rednode factory @ref rfab
 *
 * @param rfab pointer to the rednode factory to clear
 */
extern void rednode_factory_clear(rednode_factory_t *rfab);

/**
 * Sets the root component of rednode factory @ref rfab with the @ref path of given @ref length
 *
 * @param rfab   pointer to the rednode factory to set
 * @param path   the path to the root
 * @param length length of the path
 *
 * @return @ref RednodeFactory_OK on success or a negative value on error.
 * When error is returned, the absolute value returned is one of these:
 *   - @ref RednodeFactory_Error_Root_Too_Long when length exceed
 *     maximum authorized size
 *   - @ref RednodeFactory_Error_Root_Not_Absolute when path doesn't starts with '/'
 */
extern int rednode_factory_set_root_len(rednode_factory_t *rfab, const char *path, size_t length);

/**
 * Sets the root component of rednode factory @ref rfab with the @ref path
 * Synonym of @ref rednode_factory_set_root(rfab, path, strlen(path))
 *
 * @param rfab   pointer to the rednode factory to set
 * @param path   the path to the root
 *
 * @return @ref RednodeFactory_OK on success or a negative value on error.
 * When error is returned, the absolute value returned is one of these:
 *   - @ref RednodeFactory_Error_Root_Too_Long when length exceed
 *     maximum authorized size
 *   - @ref RednodeFactory_Error_Root_Not_Absolute when path doesn't starts with '/'
 */
extern int rednode_factory_set_root(rednode_factory_t *rfab, const char *path);

/**
 * Sets the node component of rednode factory @ref rfab with the @ref path of given @ref length
 *
 * @param rfab   pointer to the rednode factory to set
 * @param path   the path to the root
 * @param length length of the path
 *
 * @return @ref RednodeFactory_OK on success or a negative value on error.
 * When error is returned, the absolute value returned is one of these:
 *   - @ref RednodeFactory_Error_Node_Too_Long when length exceed
 *     maximum authorized size
 *   - @ref RednodeFactory_Error_Cleared when root wasn't already set
 */
extern int rednode_factory_set_node_len(rednode_factory_t *rfab, const char *path, size_t length);

/**
 * Sets the node component of rednode factory @ref rfab with the @ref path
 * Synonym of @ref rednode_node_set_root(rfab, path, strlen(path))
 *
 * @param rfab   pointer to the rednode factory to set
 * @param path   the path to the root
 *
 * @return @ref RednodeFactory_OK on success or a negative value on error.
 * When error is returned, the absolute value returned is one of these:
 *   - @ref RednodeFactory_Error_Node_Too_Long when length exceed
 *     maximum authorized size
 *   - @ref RednodeFactory_Error_Cleared when root wasn't already set
 */
extern int rednode_factory_set_node(rednode_factory_t *rfab, const char *path);

/**
 * Creates or updates a rednode at the path set by using @ref rednode_factory_set_root
 * and @ref rednode_factory_set_node and the parametters given in @ref params.
 *
 * When @ref update is true the node is updated if it already existed.
 *
 * @param rfab   pointer to the rednode factory to use
 * @param params parameters of creation (can be NULL for defaults)
 * @param update boolean indicating if update is possible (false enforce creation)
 * @param mode   creation mode
 * 
 * @return @ref RednodeFactory_OK on success or a negative value on error.
 * When error is returned, the absolute value returned is one of these:
 *   - @ref RednodeFactory_Error_Cleared when root wasn't already set
 *   - @ref RednodeFactory_Error_Allocation when memory got exhausted
 *   - @ref RednodeFactory_Error_Default_Alias_Empty when params->alias == NULL
 *     and when the node path is the same that the root path
 *   - @ref RednodeFactory_Error_FmtDate abnormal error when computing date
 */
extern int rednode_factory_create_node(
                rednode_factory_t *rfab,
                const rednode_factory_param_t *params,
                bool update,
                rednode_factory_mode_t mode);

#endif /* _REDNODE_FACTORY_INCLUDED_ */
