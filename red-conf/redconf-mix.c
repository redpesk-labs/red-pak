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

#define _GNU_SOURCE

#include "redconf-mix.h"

#include <stdlib.h>
#include <string.h>

#include "redconf-log.h"
#include "redconf-expand.h"
#include "redconf-sharing.h"

/****************************************************************************
* merge of path like values
* -------------------------
*
* RULE:
*  - items of child node appear before items of its parents
*  - items of admin appear before item of normal
*  - duplication is avoided
****************************************************************************/

int mixPaths(const redNodeT *node, mix_get_path_cb getpath, mix_path_cb callback, void *closure)
{
    const redNodeT *nextnode;
    const redConfigT *config;
    const char *path;
    char buffer[1024];
    int length = 0;
    int rc = 0;
    int admin = 1;

    while (rc == 0 && node != NULL) {
        if (admin) {
            admin = 0;
            config = node->confadmin;
            nextnode = node;
        }
        else {
            admin = 1;
            config = node->config;
            nextnode = node->parent;
        }
        if (config != NULL) {
            path = getpath(closure, config);
            if (path != NULL)
                rc = RedConfAppendExpandedPath(node, buffer, &length, (int)sizeof buffer, path);
        }
        node = nextnode;
    }

    if (rc == 0 && length > 0)
        rc = callback(closure, buffer, (size_t)length);
    return rc;
}

/****************************************************************************
* merge of environment variables
* ------------------------------
*
* RULE:
*  1. a normal child node can redefine an environment variable of a normal parent
*  2. an admin child node can redefine an environment variable of a parent
*
* MATRIX: (-: unset, *: any value, N: normal definition, A: admin definition
*          P: parent definition, NP: normal prent definition, AP: admin parent definition)
*
*   PARENT  NORMAL ADMIN EFFECT
*     -       N      -     N
*     P       -      -     P
*     NP      N      -     N - WARN
*     AP      N      -     AP
*     *       *      A     A - WARN
****************************************************************************/

typedef struct mix_var_link_s mix_var_link_t;

/**
 * structure for merging variables
 * this structure is intended to leave in stack
 * it records the setting of a variable
 */
struct mix_var_link_s
{
    /* link to an other variable */
    mix_var_link_t *next;
    /* definition of the variable */
    const redConfVarT *var;
    /** node where lastly defined */
    const redNodeT *node;
    /** depth of the definition */
    unsigned depth;
    /** is an admin setting? */
    unsigned admin;
};

/**
 * structure holding state of variable merging
 */
typedef struct
{
    /** count of variables to set */
    unsigned count;
    /** the found variables under merge */
    mix_var_link_t *head;
    /** callback */
    mix_var_cb callback;
    /** closure of callback */
    void *closure;

}
    mix_var_root_t;

static int mergeConfVar(mix_var_root_t *merger, const redNodeT *node, unsigned admin, unsigned index, unsigned depth)
{
    const redConfVarT *dvar;
    mix_var_link_t *link;
    const redConfigT *config = admin != 0 ? node->confadmin : node->config;
    for (;;) {
        /* search a variable */
        for (;;) {
            /* search variable at current index for current config */
            if (config != NULL) {
                if (index < config->confvar_count) {
                    dvar = &config->confvar[index];
                    break;
                }
            }
            /* no variable found, try next */
            if (admin) {
                admin = 0;
                config = node->config;
            }
            else {
                node = node->parent;
                if (node == NULL) {
                    /* no more node, send result to callback */
                    mix_var_cb callback = merger->callback;
                    void *closure = merger->closure;
                    int rc = 0;
                    unsigned index = 0;
                    for (link = merger->head ; rc == 0 && link != NULL ; link = link->next)
                        rc = callback(closure, link->var, link->node, index++, merger->count);
                    return rc;
                }
                depth++;
                admin = 1;
                config = node->confadmin;
            }
            index = 0;
        }
        /* index is now for next variable */
        index++;
        /* search if variable already seen */
        link = merger->head;
        while (link != NULL && strcasecmp(link->var->key, dvar->key) != 0)
            link = link->next;
        if (link == NULL) {
            /* variable not already seen */
            mix_var_link_t item;
            item.next = merger->head;
            item.var = dvar;
            item.node = node;
            item.depth = depth;
            item.admin = admin;
            merger->head = &item;
            merger->count++;
            return mergeConfVar(merger, node, admin, index, depth);
        }
        /* variable already seen, perform back-merging here */
        if (link->depth == depth) {
            /* same depth means same node */
            if (link->admin == admin) {
                /* this is an error because the same variable is defined 2 times */
                RedLog(REDLOG_WARNING,"Variable %s is defined twice in %s%s",
                            dvar->key, node->redpath, admin ? " (admin)" : "");
                if (link->var->mode != dvar->mode
                 || (link->var->value != NULL && dvar->value != NULL && strcmp(link->var->value, dvar->value) != 0)) {
                    RedLog(REDLOG_ERROR,"Variable %s is defined twice and differently in %s%s",
                                dvar->key, node->redpath, admin ? " (admin)" : "");
                    return -1;
                }
            }
            else {
                /*
                * it is assumed here that admin == 0 and link->admin == 1
                * do nothing because admin is allowed to redefine the value
                * and the value is set by the admin
                */
            }
        }
        else {
            /*
            * different depth means different nodes
            * it is assumed that current dvar is in parent and was overwriten by link child
            */
            if (admin) {
                if (link->admin) {
                    /* overwrite allowed, just show warning */
                    if (dvar->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Variable %s of %s (admin) redefined in %s (admin): %s",
                                        dvar->key, node->redpath, link->node->redpath, dvar->warn);
                    }
                }
                else {
                    /* for administration, normal settings are ignored */
                    link->admin = admin;
                    link->depth = depth;
                    link->node = node;
                    link->var = dvar;
                }
            }
            else {
                /* overwrite allowed, just show warning */
                if (link->admin == 0) {
                    /* same or weaker value, just show warning */
                    if (dvar->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Variable %s of %s redefined in %s: %s",
                                        dvar->key, node->redpath, link->node->redpath, dvar->warn);
                    }
                }
            }
        }
    }
}

int mixVariables(const redNodeT *node, mix_var_cb callback, void *closure)
{
    mix_var_root_t merger = {
        .count = 0,
        .head = NULL,
        .callback = callback,
        .closure = closure
    };
    return mergeConfVar(&merger, node, 1, 0, 0);
}

/****************************************************************************
* merge of capabilities
* ---------------------
*
* RULE:
*  1. a child node can not enable a capability disabled by a parent
*  2. an admin config can enable a capability disabled by a normal node
*  3. when admin is required, a child normal node can't change admin parent settings
*
* MATRIX: (-: unset, *: any value, 0N: normal disabled, 1N: normal enabled,
*          0A: admin disabled, 1A: admin enabled)
*
*   PARENT  NORMAL ADMIN EFFECT
*     -       -      -     -
*     -       0      -     0N
*     -       1      -     1N
*     -       *      0     0A
*     -       *      1     1A
*
*     0N      -      -     0N
*     0N      0      -     0N
*     0N      1      -     ERROR
*     0N      *      0     0A
*     0N      *      1     1A
*
*     1N      -      -     1N
*     1N      0      -     0N - WARN
*     1N      1      -     1N - WARN
*     1N      *      0     0A
*     1N      *      1     1A
*
*     0A      *      -     0A
*     0A      *      0     0A - WARN
*     0A      *      1     ERROR
*
*     1A      *      -     1A
*     1A      *      0     0A - WARN
*     1A      *      1     1A - WARN
****************************************************************************/

typedef struct mix_capa_link_s mix_capa_link_t;

/**
 * structure for merging capabilities
 * this structure is intended to leave in stack
 * it records the setting of a capability
 */
struct mix_capa_link_s
{
    /* link to an other capability */
    mix_capa_link_t *next;
    /** name of the capability */
    const char *name;
    /** node where lastly defined */
    const redNodeT *node;
    /** current definition value */
    int value;
    /** depth of the definition */
    unsigned depth;
    /** is an admin setting? */
    unsigned admin;
};

/**
 * structure holding state of capability merging
 */
typedef struct
{
    /** count of capabilities to set */
    unsigned count;
    /** the found capabilities under merge */
    mix_capa_link_t *head;
    /** callback */
    mix_capa_cb callback;
    /** closure of callback */
    void *closure;

}
    mix_capa_root_t;

static int mergeConfCapa(mix_capa_root_t *merger, const redNodeT *node, unsigned admin, unsigned index, unsigned depth)
{
    const redConfCapT *dcap;
    mix_capa_link_t *link;
    const redConfigT *config = admin != 0 ? node->confadmin : node->config;
    for (;;) {
        /* search a capability */
        for (;;) {
            /* search capability at current index for current config */
            if (config != NULL) {
                const redConfTagT *conftag = &config->conftag;
                if (conftag != NULL && index < (unsigned)conftag->capabilities_count) {
                    dcap = &conftag->capabilities[index];
                    break;
                }
            }
            /* no capability found, try next */
            if (admin) {
                admin = 0;
                config = node->config;
            }
            else {
                node = node->parent;
                if (node == NULL) {
                    /* no more node, send result to callback */
                    mix_capa_cb callback = merger->callback;
                    void *closure = merger->closure;
                    int rc = 0;
                    unsigned index = 0;
                    for (link = merger->head ; rc == 0 && link != NULL ; link = link->next)
                        rc = callback(closure, link->name, link->value, index++, merger->count);
                    return rc;
                }
                depth++;
                admin = 1;
                config = node->confadmin;
            }
            index = 0;
        }
        /* index is now for next capability */
        index++;
        /* search if capability already seen */
        link = merger->head;
        while (link != NULL && strcasecmp(link->name, dcap->cap) != 0)
            link = link->next;
        if (link == NULL) {
            /* capability not already seen */
            mix_capa_link_t item;
            item.next = merger->head;
            item.name = dcap->cap;
            item.node = node;
            item.value = dcap->add;
            item.depth = depth;
            item.admin = admin;
            merger->head = &item;
            merger->count++;
            return mergeConfCapa(merger, node, admin, index, depth);
        }
        /* capability already seen, perform back-merging here */
        if (link->depth == depth) {
            /* same depth means same node */
            if (link->admin == admin) {
                /* this is an error because the same capability is defined 2 times */
                RedLog(REDLOG_WARNING,"Capability %s is defined twice in %s%s",
                            link->name, node->redpath, admin ? " (admin)" : "");
                if (link->value != dcap->add) {
                    RedLog(REDLOG_ERROR,"Capability %s is defined twice and differently in %s%s",
                                link->name, node->redpath, admin ? " (admin)" : "");
                    return -1;
                }
            }
            else {
                /*
                * it is assumed here that admin == 0 and link->admin == 1
                * do nothing because admin is allowed to redefine the value
                * and the value is set by the admin
                */
            }
        }
        else {
            /*
            * different depth means different node
            * it is assumed that current dcap is in parent and was overwriten by link child
            */
            if (admin) {
                if (link->admin) {
                    /* check admin overwrite */
                    if (link->value != 0 && dcap->add == 0) {
                        RedLog(REDLOG_ERROR, "Capability %s of %s (admin) badly redefined in %s (admin)",
                                        dcap->cap, node->redpath, link->node->redpath);
                        return -1;
                    }

                    /* same or weaker value, just show warning */
                    if (dcap->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Capability %s of %s (admin) redefined in %s (admin): %s",
                                        dcap->cap, node->redpath, link->node->redpath, dcap->warn);
                    }
                }
                else {
                    /* for administration, normal settings are ignored */
                    link->value = dcap->add;
                    link->admin = admin;
                    link->depth = depth;
                    link->node = node;
                }
            }
            else {
                if (link->admin) {
                    /* do nothing because admin can overwrite normal setting */
                }
                else {
                    if (link->value != 0 && dcap->add == 0) {
                        RedLog(REDLOG_ERROR, "Capability %s of %s badly redefined in %s",
                                        dcap->cap, node->redpath, link->node->redpath);
                        return -1;
                    }

                    /* same or weaker value, just show warning */
                    if (dcap->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Capability %s of %s redefined in %s: %s",
                                        dcap->cap, node->redpath, link->node->redpath, dcap->warn);
                    }
                }
            }
        }
    }
}

int mixCapabilities(const redNodeT *node, mix_capa_cb callback, void *closure)
{
    mix_capa_root_t merger = {
        .count = 0,
        .head = NULL,
        .callback = callback,
        .closure = closure
    };
    return mergeConfCapa(&merger, node, 1, 0, 0);
}

/****************************************************************************
* merge of exports
* ----------------
*
* RULE:
*  1. a normal child node can redefine an export of a normal parent
*  2. an admin child node can redefine an export of a parent
*
* MATRIX: (-: unset, *: any value, N: normal definition, A: admin definition
*          P: parent definition, NP: normal prent definition, AP: admin parent definition)
*
*   PARENT  NORMAL ADMIN EFFECT
*     -       N      -     N
*     P       -      -     P
*     NP      N      -     N - WARN
*     AP      N      -     AP
*     *       *      A     A - WARN
****************************************************************************/

typedef struct mix_exp_link_s mix_exp_link_t;

/**
 * structure for merging exports
 * this structure is intended to leave in stack
 * it records the setting of a export
 */
struct mix_exp_link_s
{
    /* link to an other export */
    mix_exp_link_t *next;
    /* definition of the export */
    const redConfExportPathT *exp;
    /** node where lastly defined */
    const redNodeT *node;
    /** depth of the definition */
    unsigned depth;
    /** is an admin setting? */
    unsigned admin;
};

/**
 * structure holding state of export merging
 */
typedef struct
{
    /** count of exports to set */
    unsigned count;
    /** the found exports under merge */
    mix_exp_link_t *head;
    /** callback */
    mix_exp_cb callback;
    /** closure of callback */
    void *closure;

}
    mix_exp_root_t;

static int cmp_exp_link(const void *a, const void *b)
{
    mix_exp_link_t *link_a = *(mix_exp_link_t**)a;
    mix_exp_link_t *link_b = *(mix_exp_link_t**)b;
    return strcmp(link_a->exp->mount, link_b->exp->mount);
}

static int mergeConfExpEnd(mix_exp_root_t *merger)
{
    int rc = 0;
    unsigned index = 0;
    unsigned count = merger->count;
    mix_exp_link_t *link = merger->head;
    mix_exp_link_t *links[count];

    /* sort paths */
    for (; link != NULL; link = link->next)
        links[index++] = link;
    qsort(links, count, sizeof links[0], cmp_exp_link);

    /* send to callback in order */
    for (index = 0 ; rc == 0 && index < count ; index++)
        rc = merger->callback(merger->closure, links[index]->exp, links[index]->node, index, count);

    return rc;
}

static int mergeConfExp(mix_exp_root_t *merger, const redNodeT *node, unsigned admin, unsigned index, unsigned depth)
{
    const redConfExportPathT *dexp;
    mix_exp_link_t *link;
    const redConfigT *config = admin != 0 ? node->confadmin : node->config;
    for (;;) {
        /* search a export */
        for (;;) {
            /* search export at current index for current config */
            if (config != NULL) {
                if (index < config->exports_count) {
                    dexp = &config->exports[index];
                    /* ignore private inherited exports */
                    if (!(depth > 0 && (dexp->mode & RED_EXPORT_PRIVATES) != 0))
                        break;
                    index++;
                    continue;
                }
            }
            /* no export found, try next */
            if (admin) {
                admin = 0;
                config = node->config;
            }
            else {
                node = node->parent;
                if (node == NULL)
                    /* no more node, send result to callback */
                    return mergeConfExpEnd(merger);
                depth++;
                admin = 1;
                config = node->confadmin;
            }
            index = 0;
        }
        /* index is now for next export */
        index++;
        /* search if export already seen */
        link = merger->head;
        while (link != NULL && strcasecmp(link->exp->mount, dexp->mount) != 0)
            link = link->next;
        if (link == NULL) {
            /* export not already seen */
            mix_exp_link_t item;
            item.next = merger->head;
            item.exp = dexp;
            item.node = node;
            item.depth = depth;
            item.admin = admin;
            merger->head = &item;
            merger->count++;
            return mergeConfExp(merger, node, admin, index, depth);
        }
        /* export already seen, perform back-merging here */
        if (link->depth == depth) {
            /* same depth means same node */
            if (link->admin == admin) {
                /* this is an error because the same export is defined 2 times */
                RedLog(REDLOG_WARNING,"Export %s is defined twice in %s%s",
                            dexp->mount, node->redpath, admin ? " (admin)" : "");
                if (link->exp->mode != dexp->mode
                 || (link->exp->path != NULL && dexp->path != NULL && strcmp(link->exp->path, dexp->path) != 0)) {
                    RedLog(REDLOG_ERROR,"Export %s is defined twice and differently in %s%s",
                                dexp->mount, node->redpath, admin ? " (admin)" : "");
                    return -1;
                }
            }
            else {
                /*
                * it is assumed here that admin == 0 and link->admin == 1
                * do nothing because admin is allowed to redefine the value
                * and the value is set by the admin
                */
            }
        }
        else {
            /*
            * different depth means different nodes
            * it is assumed that current dexp is in parent and was overwriten by link child
            */
            if (admin) {
                if (link->admin) {
                    /* overwrite allowed, just show warning */
                    if (dexp->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Export %s of %s (admin) redefined in %s (admin): %s",
                                        dexp->mount, node->redpath, link->node->redpath, dexp->warn);
                    }
                }
                else {
                    /* for administration, normal settings are ignored */
                    link->admin = admin;
                    link->depth = depth;
                    link->node = node;
                    link->exp = dexp;
                }
            }
            else {
                /* overwrite allowed, just show warning */
                if (link->admin == 0) {
                    /* same or weaker value, just show warning */
                    if (dexp->warn != NULL) {
                        RedLog(REDLOG_WARNING, "Export %s of %s redefined in %s: %s",
                                        dexp->mount, node->redpath, link->node->redpath, dexp->warn);
                    }
                }
            }
        }
    }
}

int mixExports(const redNodeT *node, mix_exp_cb callback, void *closure)
{
    mix_exp_root_t merger = {
        .count = 0,
        .head = NULL,
        .callback = callback,
        .closure = closure
    };
    return mergeConfExp(&merger, node, 1, 0, 0);
}

/****************************************************************************
* merge of Sharings
* -----------------
*
* RULE:
*  1. a child node can not enable a sharing disabled by a parent
*  2. an admin config can enable a sharing disabled by a normal node
*  3. when admin is required, a child normal node can't change admin parent settings
*
* MATRIX: (-: unset, *: any value, 0N: normal disabled, 1N: normal enabled or joining,
*          0A: admin disabled, 1A: admin enabled or joining)
*
*   PARENT  NORMAL ADMIN EFFECT
*     -       -      -     -
*     -       0      -     0N
*     -       1      -     1N
*     -       *      0     0A
*     -       *      1     1A
*
*     0N      -      -     0N
*     0N      0      -     0N
*     0N      1      -     ERROR
*     0N      *      0     0A
*     0N      *      1     1A
*
*     1N      -      -     1N
*     1N      0      -     0N
*     1N      1      -     1N
*     1N      *      0     0A
*     1N      *      1     1A
*
*     0A      *      -     0A
*     0A      *      0     0A
*     0A      *      1     ERROR
*
*     1A      *      -     1A
*     1A      *      0     0A
*     1A      *      1     1A
****************************************************************************/

typedef
struct {
    /* current value */
    const char *value;
    /* current type */
    redConfSharingE type;
    /* for admin? */
    int admin;
    /* node where defined */
    const redNodeT *node;
}
    mix_share_t;

typedef
struct
{
    mix_share_t all;
    mix_share_t user;
    mix_share_t cgroup;
    mix_share_t ipc;
    mix_share_t pid;
    mix_share_t net;
    mix_share_t time;
}
    mix_sharings_t;

static void init_mix_sharings(mix_sharings_t *sharings)
{
    memset(sharings, 0, sizeof *sharings);
}

static redConfShareT mix_sharings_get(const mix_sharings_t *sharings)
{
    return (redConfShareT){
        .all     = sharings->all.value,
        .user    = sharings->user.value   ?: sharings->all.value,
        .cgroup  = sharings->cgroup.value ?: sharings->all.value,
        .ipc     = sharings->ipc.value    ?: sharings->all.value,
        .pid     = sharings->pid.value    ?: sharings->all.value,
        .net     = sharings->net.value    ?: sharings->all.value,
        .time    = sharings->time.value
    };
}

static int mix_share(mix_share_t *share, const char *value, const redNodeT *node, int admin, const char *name)
{
    /* get value type */
    redConfSharingE type = sharing_type(value);

    /* no change */
    if (type == RED_CONF_SHARING_UNSET)
        return 0;

    /* set it if unset or overwriten by admin */
    if (share->type == RED_CONF_SHARING_UNSET || (admin && !share->admin)) {
        share->value = value;
        share->type = type;
        share->admin = admin;
        share->node = node;
        return 0;
    }

    /* normal doesn't alter admin */
    if (!admin && share->admin)
        return 0;

    /* if equal */
    if (share->type == type && (type != RED_CONF_SHARING_JOIN || strcasecmp(value, share->value) == 0))
        return 0;

    /* different values, forbids sharing is previously forbidden */
    if (type != RED_CONF_SHARING_DISABLED)
        return 0;

    RedLog(REDLOG_ERROR,"Can't share %s in %s%s because forbidden in %s%s",
                name,
                share->node->redpath, admin ? " (admin)" : "",
                node->redpath, admin ? " (admin)" : "");
    return -1;
}

static int mix_sharings(mix_sharings_t *sharings, redConfShareT *consha, const redNodeT *node, int admin)
{
    return mix_share(&sharings->all,    consha->all,    node, admin, "all")
        ?: mix_share(&sharings->user,   consha->user,   node, admin, "user")
        ?: mix_share(&sharings->cgroup, consha->cgroup, node, admin, "cgroup")
        ?: mix_share(&sharings->ipc,    consha->ipc,    node, admin, "ipc")
        ?: mix_share(&sharings->pid,    consha->pid,    node, admin, "pid")
        ?: mix_share(&sharings->net,    consha->net,    node, admin, "net")
        ?: mix_share(&sharings->time,   consha->time,   node, admin, "time");
}

int mixShares(const redNodeT *node, mix_share_cb callback, void *closure)
{
    int rc = 0;
    mix_sharings_t sharings;

    init_mix_sharings(&sharings);
    while (rc == 0 && node != NULL) {
        if (node->confadmin)
            rc = mix_sharings(&sharings, &node->confadmin->conftag.share, node, 1);
        if (rc == 0 && node->config)
            rc = mix_sharings(&sharings, &node->config->conftag.share, node, 0);
        node = node->parent;
    }
    if (rc == 0) {
        redConfShareT share = mix_sharings_get(&sharings);
        rc = callback(closure, &share);
    }
    return rc;
}


/****************************************************************************
* merge of conftag
* ----------------
*
* RULE:
*  1. sharings are merged as sharings
*  2. cgroups .....
*  3. other values are set by the last item
*
* MATRIX: (-: unset, *: any value, N: normal definition, A: admin definition
*          P: parent definition, NP: normal prent definition, AP: admin parent definition)
*
*   PARENT  NORMAL ADMIN EFFECT
*     P       -      -     P
*     -       N      -     N
*     NP      N      -     N
*     AP      N      -     AP
*     *       *      A     A
****************************************************************************/

/*
 * merge the source string to the destination
 * returns 0 on success or 1 on error
 */
static inline int mix_string(const char **destination, const char *source, const redNodeT *node) {
    if (source != NULL && *destination == NULL) {
        *destination = expandAlloc(node, source, 1);
        if (*destination == NULL)
            return -1;
    }
    return 0;
}

/* merge the source opt flag to the destination */
static inline void mix_optflag_overwrite(redConfOptFlagE *destination, redConfOptFlagE source) {
    if (*destination == RED_CONF_OPT_UNSET)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void mix_int_not_null(int *destination, int source) {
    if (*destination == 0)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void mix_uint_not_null(unsigned *destination, unsigned source) {
    if (*destination == 0)
        *destination = source;
}

/* merge the source opt flag to the destination */
static inline void mix_bool_not_null(unsigned *destination, unsigned source) {
    if (*destination == 0)
        *destination = source;
}

static int mix_basic_conftag(redConfTagT *destination, const redConfTagT *source, const redNodeT *node)
{
    mix_optflag_overwrite(&destination->diewithparent, source->diewithparent);
    mix_optflag_overwrite(&destination->newsession, source->newsession);
    mix_int_not_null(&destination->verbose, source->verbose);
    mix_bool_not_null(&destination->inheritenv, source->inheritenv);
    mix_uint_not_null(&destination->maprootuser, source->maprootuser);
    return mix_string(&destination->cachedir, source->cachedir, node)
        || mix_string(&destination->hostname, source->hostname, node)
        || mix_string(&destination->chdir, source->chdir, node)
        || mix_string(&destination->umask, source->umask, node)
        || mix_string(&destination->setuser, source->setuser, node)
        || mix_string(&destination->setgroup, source->setgroup, node)
        || mix_string(&destination->cgrouproot, source->cgrouproot, node);
}

static int set_conftag_share(void *closure, const redConfShareT *shares)
{
    redConfShareT *dest = closure;
    *dest = *shares;
    return 0;
}

static int mix_conftag_capa(void *closure, const char *capability, int value, unsigned index, unsigned count)
{
    redConfTagT *conftag = closure;
    if (index == 0) {
        conftag->capabilities = calloc(count, sizeof *conftag->capabilities);
        if (conftag->capabilities == NULL)
            return -1;
        conftag->capabilities_count = count;
    }
    conftag->capabilities[index].cap = capability;
    conftag->capabilities[index].add = value;
    return 0;
}

int mixConfTags(const redNodeT *node, mix_conftag_cb callback, void *closure)
{
    const redNodeT *iter;
    redConfTagT conftag;
    int rc = 0;

    memset(&conftag, 0, sizeof conftag);

    for (iter = node ; rc >= 0 && iter != NULL ; iter = iter->parent)
        if (iter->confadmin)
            rc = mix_basic_conftag(&conftag, &iter->confadmin->conftag, node);

    for (iter = node ; rc >= 0 && iter != NULL ; iter = iter->parent)
        if (iter->config)
            rc = mix_basic_conftag(&conftag, &iter->config->conftag, node);

    if (rc >= 0)
        rc = mixShares(node, set_conftag_share, &conftag.share);

    if (rc >= 0)
        rc = mixCapabilities(node, mix_conftag_capa, &conftag);

    if (rc >= 0)
        rc = callback(closure, &conftag);

    free((void*)conftag.cachedir);
    free((void*)conftag.hostname);
    free((void*)conftag.chdir);
    free((void*)conftag.umask);
    free((void*)conftag.cgrouproot);
    free((void*)conftag.capabilities);

    return rc;
}

/****************************************************************************
* merge of early conf
* -------------------
*
* RULE:
*  1. values are set by the last item
*
* MATRIX: (-: unset, *: any value, N: normal definition, A: admin definition
*          P: parent definition, NP: normal prent definition, AP: admin parent definition)
*
*   PARENT  NORMAL ADMIN EFFECT
*     P       -      -     P
*     -       N      -     N
*     NP      N      -     N
*     AP      N      -     AP
*     *       *      A     A
****************************************************************************/

static inline void mix_const_string(const char **destination, const char *source) {
    if (source != NULL && *destination == NULL)
        *destination = source;
}

static void mix_early_conf(early_conf_t *conf, const redConfigT *nodeconf)
{
    mix_const_string(&conf->setuser, nodeconf->conftag.setuser);
    mix_const_string(&conf->setgroup, nodeconf->conftag.setgroup);
}

int mixEarlyConf(const redNodeT *node, mix_early_conf_cb callback, void *closure)
{
    const redNodeT *iter;
    early_conf_t conf;

    memset(&conf, 0, sizeof conf);

    for (iter = node ; iter != NULL ; iter = iter->parent)
        if (iter->confadmin)
            mix_early_conf(&conf, iter->confadmin);

    for (iter = node ; iter != NULL ; iter = iter->parent)
        if (iter->config)
            mix_early_conf(&conf, iter->config);

    return callback(closure, &conf, node);
}

