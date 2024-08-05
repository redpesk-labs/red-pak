/*
* Copyright (C) 2020 "IoT.bzh"
* Author Fulup Ar Foll <fulup@iot.bzh>
* Author Clément Bénier <clement.benier@iot.bzh>
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
* Documentation: https://github.com/tlsa/libcyaml/blob/master/docs/guide.md
*/
#define _GNU_SOURCE
#include <rpm/rpmlog.h>
#include "redconf.h"

// shorten schema line lenght
#define CYFLAG_PTR  CYAML_FLAG_POINTER
#define CYFLAG_CASE CYAML_FLAG_CASE_INSENSITIVE
#define CYFLAG_OPT  CYAML_FLAG_OPTIONAL

#define RED_FIELD_INFO(_STRUCTTYPE) CYAML_FIELD_STRING_PTR("info", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, _STRUCTTYPE, info, 0, CYAML_UNLIMITED)
#define RED_FIELD_WARN(_STRUCTTYPE) CYAML_FIELD_STRING_PTR("warn", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, _STRUCTTYPE, warn, 0, CYAML_UNLIMITED)

static int LOGYAML = 0;

static const cyaml_config_t yconfError= {
    .log_level = CYAML_LOG_ERROR,
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
};

static const cyaml_config_t yconfWarning = {
    .log_level = CYAML_LOG_WARNING,
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
};

static const cyaml_config_t yconfNotice = {
    .log_level = CYAML_LOG_NOTICE,
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
};

static const cyaml_config_t yconfInfo = {
    .log_level = CYAML_LOG_INFO,
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
};

static const cyaml_config_t yconfDebug= {
    .log_level = CYAML_LOG_DEBUG,
    .log_fn = cyaml_log,
    .mem_fn = cyaml_mem,
};

static const cyaml_config_t *yconft[] ={
   &yconfError,
   &yconfWarning,
   &yconfInfo,
   &yconfNotice,
   &yconfDebug,
};

static const cyaml_config_t *yconfGet (int wlevel) {
    static int maxLevel = (int) (sizeof(yconft) / sizeof(cyaml_config_t*)) -1;

    if (wlevel < LOGYAML)
        wlevel = LOGYAML;

    if (wlevel > maxLevel) {
        rpmlog(REDLOG_ERROR, "Fail yconf verbosity wlevel too high val=%d max=%d", wlevel, maxLevel);
        return NULL;
    }
    return yconft[wlevel];
}

/****************************************************************************************************************
* --- CONFIG SECTION --- *
*****************************************************************************************************************/

const cyaml_strval_t redConfOptStrings[] ={
   {"Unset"  , RED_CONF_OPT_UNSET},
   {"Enabled", RED_CONF_OPT_ENABLED},
   {"Disabled",RED_CONF_OPT_DISABLED},
};

/**** cgroup ****/
static const cyaml_schema_field_t CgroupsMem[] = {
    CYAML_FIELD_STRING_PTR("max", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, max, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("high", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, high, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("min", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, min, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("low", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, low, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("oom_group", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, oom_group, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("swap_high", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, swap_high, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("swap_max", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redMemT, swap_max, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t CgroupsCpu[] = {
    CYAML_FIELD_STRING_PTR("weight", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpuT, weight, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("weight_nice", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpuT, weight_nice, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("max", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpuT, max, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t CgroupsCpuSet[] = {
    CYAML_FIELD_STRING_PTR("cpus", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpusetT, cpus, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("mems", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpusetT, mems, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("cpus_partition", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redCpusetT, cpus_partition, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t CgroupsIo[] = {
    CYAML_FIELD_STRING_PTR("cost_qos", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redIoT, cost_qos, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("cost_model", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redIoT, cost_model, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("weight", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redIoT, weight, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("max", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redIoT, max, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t CgroupsPids[] = {
    CYAML_FIELD_STRING_PTR("max", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redPidsT, max, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_field_t CgroupsSchema[] = {
    CYAML_FIELD_MAPPING_PTR("cpuset", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfCgroupT, cpuset, CgroupsCpuSet),
    CYAML_FIELD_MAPPING_PTR("mem", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfCgroupT , mem, CgroupsMem),
    CYAML_FIELD_MAPPING_PTR("cpu", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfCgroupT , cpu, CgroupsCpu),
    CYAML_FIELD_MAPPING_PTR("io", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfCgroupT , io, CgroupsIo),
    CYAML_FIELD_MAPPING_PTR("pids", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfCgroupT , pids, CgroupsPids),
    CYAML_FIELD_END
};
/**** end cgroup ****/

/**** capabilities ****/
static const cyaml_schema_field_t CapabilityEntry[] = {
    CYAML_FIELD_STRING_PTR("cap", CYFLAG_PTR|CYFLAG_CASE, redConfCapT, cap, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("add", CYFLAG_PTR|CYFLAG_CASE, redConfCapT, add),
    RED_FIELD_INFO(redConfCapT),
    RED_FIELD_WARN(redConfCapT),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t CapabilitiesSchema= {CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,redConfCapT, CapabilityEntry),};
/**** end capabilities ****/

/* config main part */
static const cyaml_schema_field_t ConfigSchema[] = {
    CYAML_FIELD_STRING_PTR("persistdir", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, persistdir, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("rpmdir", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, rpmdir, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("cachedir", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, cachedir, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("path", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, path, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("ldpath", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, ldpath, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("umask", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, umask, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("verbose", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, verbose),
    CYAML_FIELD_INT("maxage", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, maxage),
    CYAML_FIELD_INT("map-root-user", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, maprootuser),
    CYAML_FIELD_BOOL("gpgcheck", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, gpgcheck),
    CYAML_FIELD_BOOL("inherit", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, inherit),
    CYAML_FIELD_BOOL("unsafe", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, unsafe),
    CYAML_FIELD_ENUM("die-with-parent", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, diewithparent, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("new-session", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, newsession, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_all", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_all, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_user", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_user, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_cgroup", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_cgroup, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_net", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_net, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_pid", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_pid, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_ipc", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_ipc, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_ENUM("share_time", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, share_time, redConfOptStrings, CYAML_ARRAY_LEN(redConfOptStrings)),
    CYAML_FIELD_MAPPING_PTR("cgroups", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, cgroups, CgroupsSchema),
    CYAML_FIELD_STRING_PTR("hostname", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, hostname, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("chdir", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, chdir, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("cgrouproot", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, cgrouproot, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("capabilities", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfTagT, capabilities, &CapabilitiesSchema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

/****************************************************************************************************************
* --- EXPORT SECTION --- *
*****************************************************************************************************************/

const cyaml_strval_t exportFlagStrings[] = {
    { "Restricted",     RED_EXPORT_RESTRICTED},
    { "Public",         RED_EXPORT_PUBLIC},
    { "Private",        RED_EXPORT_PRIVATE},
    { "PrivateRestricted",        RED_EXPORT_PRIVATE_RESTRICTED},
    { "RestrictedFile", RED_EXPORT_RESTRICTED_FILE},
    { "PublicFile",     RED_EXPORT_PUBLIC_FILE},
    { "PrivateFile",    RED_EXPORT_PRIVATE_FILE},
    { "Anonymous",      RED_EXPORT_ANONYMOUS},
    { "Symlink",        RED_EXPORT_SYMLINK},
    { "Execfd" ,        RED_EXPORT_EXECFD},
    { "Internal" ,      RED_EXPORT_DEFLT},
    { "Tmpfs"    ,      RED_EXPORT_TMPFS},
    { "Procfs"   ,      RED_EXPORT_PROCFS},
    { "Mqueue"   ,      RED_EXPORT_MQUEFS},
    { "Devfs"    ,      RED_EXPORT_DEVFS},
    { "Lock"     ,      RED_EXPORT_LOCK},
};

const cyaml_strval_t redVarEnvStrings[] = {
    {"Static",   RED_CONFVAR_STATIC},
    {"Execfd",   RED_CONFVAR_EXECFD},
    {"Default",  RED_CONFVAR_DEFLT},
    {"Remove",   RED_CONFVAR_REMOVE},
};

// mounting point label+path
static const cyaml_schema_field_t ExportEntry[] = {
    CYAML_FIELD_ENUM("mode", CYAML_FLAG_STRICT, redConfExportPathT, mode, exportFlagStrings, CYAML_ARRAY_LEN(exportFlagStrings)),
    CYAML_FIELD_STRING_PTR("mount", CYFLAG_PTR|CYFLAG_CASE, redConfExportPathT, mount, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("path", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfExportPathT, path, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("info", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfExportPathT, info, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("warn", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfExportPathT, warn, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

// relocation old/new path
static const cyaml_schema_field_t RelocationEntry[] = {
    CYAML_FIELD_STRING_PTR("old", CYFLAG_PTR|CYFLAG_CASE, redConfRelocationT, old, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("new", CYFLAG_PTR|CYFLAG_CASE, redConfRelocationT, tnew, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

// relocation old/new path
static const cyaml_schema_field_t EnvValEntry[] = {
    CYAML_FIELD_ENUM("mode", CYAML_FLAG_STRICT|CYFLAG_OPT, redConfVarT, mode, redVarEnvStrings, CYAML_ARRAY_LEN(redVarEnvStrings)),
    CYAML_FIELD_STRING_PTR("key", CYFLAG_PTR|CYFLAG_CASE, redConfVarT, key, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("value", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfVarT, value, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("info", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfVarT, info, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("warn", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfVarT, warn, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END
};

static const cyaml_schema_value_t ExportSchema= {CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,redConfExportPathT, ExportEntry),};
static const cyaml_schema_value_t RelocsSchema= {CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,redConfRelocationT, RelocationEntry),};
static const cyaml_schema_value_t EnvVarSchema= {CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,redConfVarT, EnvValEntry),};

/****************************************************************************************************************
* --- HEADER SECTION --- *
*****************************************************************************************************************/

    // redpak config headers schema
    static const cyaml_schema_field_t HeaderSchema[] = {
        CYAML_FIELD_STRING_PTR("alias", CYFLAG_PTR|CYFLAG_CASE, redConfHeaderT, alias, 0, CYAML_UNLIMITED),
        CYAML_FIELD_STRING_PTR("name", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfHeaderT, name, 0, CYAML_UNLIMITED),
        CYAML_FIELD_STRING_PTR("info", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfHeaderT, info, 0,CYAML_UNLIMITED),
        CYAML_FIELD_END
    };

/****************************************************************************************************************
* --- SCHEMA MAIN ENTRY --- *
*****************************************************************************************************************/

// First wlevel config structure (id, export, acl, status)
static const cyaml_schema_field_t TopLevelSchema[] = {
    CYAML_FIELD_MAPPING_PTR("headers", CYFLAG_PTR|CYFLAG_CASE, redConfigT , headers, HeaderSchema),
    CYAML_FIELD_SEQUENCE("exports", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfigT , exports, &ExportSchema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("relocations", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfigT , relocations, &RelocsSchema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("environ", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfigT , confvar, &EnvVarSchema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_MAPPING_PTR("config", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redConfigT , conftag, ConfigSchema),
    CYAML_FIELD_END
};

// Top wlevel schema entry point must be a unique CYAML_VALUE_MAPPING
static const cyaml_schema_value_t RedPakConfigSchema = {
    CYAML_VALUE_MAPPING(CYFLAG_PTR|CYFLAG_CASE, redConfigT, TopLevelSchema),
};

/****************************************************************************************************************
* ---- END <redpath>/etc/redpack.yaml SCHEMA ----
*****************************************************************************************************************/

/****************************************************************************************************************
* --- STATUS SCHEMA --- .rednode.yaml file --- *
*****************************************************************************************************************/

const cyaml_strval_t statusFlagStrings[] = {
    { "Disable", RED_STATUS_DISABLE},
    { "Enable",  RED_STATUS_ENABLE},
    { "Unknown", RED_STATUS_UNKNOWN},
    { "Error",   RED_STATUS_ERROR},
};

// status data
static const cyaml_schema_field_t StatusEntry[] = {    /// trouve le bon type pour count check les tasks
    CYAML_FIELD_STRING_PTR("realpath", CYFLAG_PTR|CYFLAG_CASE, redStatusT, realpath, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("info", CYFLAG_PTR|CYFLAG_CASE|CYFLAG_OPT, redStatusT, info, 0, CYAML_UNLIMITED),
    CYAML_FIELD_UINT("timestamp", CYFLAG_PTR|CYFLAG_CASE, redStatusT, timestamp),
    CYAML_FIELD_ENUM("state", CYAML_FLAG_DEFAULT, redStatusT, state, statusFlagStrings, CYAML_ARRAY_LEN(statusFlagStrings)),
    CYAML_FIELD_END
};

// Top wlevel schema entry point must be a unique CYAML_VALUE_MAPPING
static const cyaml_schema_value_t StatusTopSchema = {
    CYAML_VALUE_MAPPING(CYFLAG_PTR|CYFLAG_CASE, redConfigT, StatusEntry)
};

/****************************************************************************************************************
* SCHEMA FUNCTIONS *
*****************************************************************************************************************/

static int SchemaSave (const char* filepath, const cyaml_schema_value_t *topschema, void *config, int wlevel) {
    int errcode=0;

    const cyaml_config_t *yconf = yconfGet (wlevel);
    if (!yconf) return (-1);

    errcode = cyaml_save_file(filepath, yconf, topschema, config, 0);
    if (errcode != CYAML_OK) {
        // when error reparse with higger level of debug to make visible errors
        if (wlevel <= 1)
            errcode = SchemaSave (filepath, topschema, config, 2);
        else
            rpmlog(REDLOG_ERROR, "Fail to save yaml config path=%s err=[%s]", filepath, cyaml_strerror(errcode));
    }

    return errcode;
}

static int SchemaLoad (const char* filepath, const cyaml_schema_value_t *topschema, void **config, int wlevel) {
    int errcode=0;

    // select parsing log wlevel
    const cyaml_config_t *yconf = yconfGet (wlevel);
    if (!yconf) return (-1);

    errcode = cyaml_load_file(filepath, yconf, topschema, config, 0);

    if (errcode != CYAML_OK) {
        // when error reparse with higger level of debug to make visible errors
        if (wlevel <= 1)
            errcode = SchemaLoad (filepath, topschema, config, 2);
        else
            rpmlog(REDLOG_ERROR, "Fail to reading yaml config path=%s err=[%s]", filepath, cyaml_strerror(errcode));
    }

    return errcode;
}

static int SchemaGet(char **output, size_t *len, const cyaml_schema_value_t *topschema, cyaml_data_t *data, int wlevel) {
    int errcode = 0;
    // select parsing log wlevel
    const cyaml_config_t *yconf = yconfGet (wlevel);
    if (!yconf) {
        errcode = 1;
        goto Exit;
    }

    errcode = cyaml_save_data(output, len, yconf, topschema, data, 0);
    if (errcode) {
        RedLog(REDLOG_ERROR, "issue get schema %s", cyaml_strerror(errcode));
    }

Exit:
    return errcode;
}

static int SchemaFree(const cyaml_schema_value_t *topschema, cyaml_data_t *data, int wlevel) {
    int errcode = 0;
    // select parsing log wlevel
    const cyaml_config_t *yconf = yconfGet (wlevel);
    if (!yconf) {
        errcode = 1;
        goto Exit;
    }

    errcode = cyaml_free(yconf, topschema, data, 0);
    if (errcode) {
        RedLog(REDLOG_ERROR, "issue free schema %s", cyaml_strerror(errcode));
    }

Exit:
    return errcode;
}

int RedSaveConfig (const char* filepath, redConfigT *config, int warning ) {
    int errcode = SchemaSave(filepath, &RedPakConfigSchema, (void*)config, 0);
    return errcode;
}

int RedSaveStatus (const char* filepath, redStatusT *status, int warning ) {
    int errcode = SchemaSave(filepath, &StatusTopSchema, (void*)status, 0);
    return errcode;
}

redConfigT* RedLoadConfig (const char* filepath, int warning) {
    RedLog(REDLOG_DEBUG, "load config from %s", filepath);
    redConfigT *config;
    int errcode= SchemaLoad (filepath, &RedPakConfigSchema, (void**)&config, warning);
    if (errcode) return NULL;
    return config;
}

int RedGetConfig(char **output, size_t *len, redConfigT *config) {
    return SchemaGet(output, len, &RedPakConfigSchema, config, 0);
}

int RedFreeConfig(redConfigT *config, int wlevel) {
    return SchemaFree(&RedPakConfigSchema, (cyaml_data_t *)config, 0);
}

redStatusT* RedLoadStatus (const char* filepath, int warning) {
    redStatusT *status;
    int errcode = SchemaLoad (filepath, &StatusTopSchema, (void**)&status, warning);
    if (errcode) return NULL;
    return  status;
}

int RedFreeStatus(redStatusT *status, int wlevel) {
    return SchemaFree(&StatusTopSchema, (cyaml_data_t *)status, 0);
}

int setLogYaml(int level) {
    LOGYAML = level;
    return 0;
}