#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "redconf-defaults.h"
#include "redconf-utils.h"
#include "redconf-expand.h"
#include "redconf-schema.h"
#include "redconf-node.h"
#include "rednode-factory.h"
#include "redconf-dump.h"

#ifndef TEMPLATES_DIR
#define TEMPLATES_DIR "../etc/redpak/templates.d"
#endif

/*********************************************************************/
/*********************************************************************

SOME COMMONS

*********************************************************************/
/*********************************************************************/

char tempname[50];

void make_tempname()
{
    snprintf(tempname, sizeof tempname, "/tmp/==checks-conf+%d==", (int)getpid());
}

void remove_tempfile()
{
    make_tempname();
    unlink(tempname);
}

void write_tempfile(const char *text, size_t length)
{
    ssize_t rc;
    FILE *file;

    file = fopen(tempname, "w");
    ck_assert_ptr_nonnull(file);
    rc = fwrite(text, length ? length : strlen(text), 1, file);
    ck_assert_int_eq(rc, 1);
    fclose(file);
}

/*********************************************************************/
/*********************************************************************

TEST CASE "defaults"


These tests are checking that the expansions of predefined variables
is correct and that the their default values are also correct.

The name default comes from 'redconf-defaults.c'.

*********************************************************************/
/*********************************************************************/
#if 0
#define BUFFLEN 512
int call_conf_default(const char *key, char buffer[BUFFLEN], void *arg)
{
    unsigned i = 0;
    while (nodeConfigDefaults[i].label != NULL) {
        if (!strcmp(key, nodeConfigDefaults[i].label)) {
            char *r = nodeConfigDefaults[i].callback(key, arg, nodeConfigDefaults[i].handle, buffer, BUFFLEN);
            ck_assert_ptr_eq(r, buffer);
            return 0;
        }
        i++;
    }
    return 1;
}

/* TEST expansion of variable with (or not) default values */
START_TEST(test_defaults_env)
{

#define TAG "---TAGTAG---"

    char buffer[BUFFLEN];
    unsigned i, n;
    int r;

    struct {
        const char *name;
        const char *value;
    } for_env[] = {
        {"NODE_PREFIX", NODE_PREFIX},
        {"redpak_MAIN", "$NODE_PREFIX"redpak_MAIN},
        {"redpak_TMPL", "$NODE_PREFIX"redpak_TMPL},
        {"REDNODE_CONF", "$NODE_PATH/"REDNODE_CONF},
        {"REDNODE_ADMIN", "$NODE_PATH/"REDNODE_ADMIN},
        {"REDNODE_STATUS", "$NODE_PATH/"REDNODE_STATUS},
        {"REDNODE_VARDIR", "$NODE_PATH/"REDNODE_VARDIR},
        {"REDNODE_REPODIR", "$NODE_PATH/"REDNODE_REPODIR},
        {"REDNODE_LOCK", "$NODE_PATH/"REDNODE_LOCK},
        {"LOGNAME", "Unknown"},
        {"HOSTNAME", "localhost"},
        {"CGROUPS_MOUNT_POINT", CGROUPS_MOUNT_POINT},
        {"LEAF_ALIAS", NULL},
        {"LEAF_NAME", NULL},
        {"LEAF_PATH", NULL},
        {"REDPESK_VERSION", REDPESK_DFLT_VERSION}
    };

    n = sizeof for_env / sizeof *for_env;
    for (i = 0 ; i < n ; i++) {
        setenv(for_env[i].name, TAG, 1);
        r = call_conf_default(for_env[i].name, buffer, NULL);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, TAG);
        unsetenv(for_env[i].name);
        r = call_conf_default(for_env[i].name, buffer, NULL);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, for_env[i].value ?: "#undef");
    }
}

/* TEST expansion of process system variables
   $UID, $GID, $PID */
START_TEST(test_defaults_int)
{
    char intval[BUFFLEN];
    char buffer[BUFFLEN];
    int r;

    r = call_conf_default("UID", buffer, NULL);
    ck_assert_int_eq(r, 0);
    sprintf(intval, "%lu", (unsigned long)getuid());
    ck_assert_str_eq(buffer, intval);

    r = call_conf_default("GID", buffer, NULL);
    ck_assert_int_eq(r, 0);
    sprintf(intval, "%lu", (unsigned long)getgid());
    ck_assert_str_eq(buffer, intval);

    r = call_conf_default("PID", buffer, NULL);
    ck_assert_int_eq(r, 0);
    sprintf(intval, "%lu", (unsigned long)getpid());
    ck_assert_str_eq(buffer, intval);
}


/* TEST expansion of node related variables
   $NODE_ALIAS, $NODE_NAME, $NODE_PATH, $NODE_INFO */
START_TEST(test_defaults_for_node)
{

#define TAG "---TAGTAG---"

    char buffer[BUFFLEN];
    unsigned i, n;
    int r;

    redNodeT node;
    redConfigT config;
    redConfHeaderT header;

    memset(&node, 0, sizeof node);
    memset(&config, 0, sizeof config);
    node.redpath = "---REDPATH---";
    node.config = &config;
    config.headers = &header;
    header.alias = "---alias---";
    header.name = "---name---";
    header.info = "---info---";

    struct {
        const char *name;
        const char *value;
    } for_node[] = {
        { "NODE_ALIAS", header.alias },
        { "NODE_NAME",  header.name },
        { "NODE_PATH",  node.redpath },
        { "NODE_INFO",  header.info },
    };

    n = sizeof for_node / sizeof *for_node;
    for (i = 0 ; i < n ; i++) {
        setenv(for_node[i].name, TAG, 1);
        r = call_conf_default(for_node[i].name, buffer, NULL);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, TAG);
        r = call_conf_default(for_node[i].name, buffer, &node);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, for_node[i].value);
        unsetenv(for_node[i].name);
        r = call_conf_default(for_node[i].name, buffer, NULL);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, "#undef");
        r = call_conf_default(for_node[i].name, buffer, &node);
        ck_assert_int_eq(r, 0);
        ck_assert_str_eq(buffer, for_node[i].value);
    }
}

/* TEST expansion of $UUID */
START_TEST(test_defaults_uuid)
{
    char buffer[BUFFLEN];
    int r, i;

    r = call_conf_default("UUID", buffer, NULL);
    ck_assert_int_eq(r, 0);
    ck_assert_int_eq(strlen(buffer), 36);
    for(i = 0 ; i < 36 ; i++) {
        char c = buffer[i];
        if (c == '-')
            ck_assert(i == 8 || i == 13 || i == 18 || i == 23);
        else
            ck_assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
    ck_assert(buffer[14] == '4'); /* random */
/*
    123e4567-e89b-12d3-a456-426614174000
    ........-....-M...-N...-............
    000000000011111111112222222222333333
    012345678901234567890123456789012345
*/
}

/* TEST expansion of $TODAY */
START_TEST(test_defaults_today)
{
    char m[BUFFLEN];
    char z[BUFFLEN];
    char buffer[BUFFLEN];
    int r, d, y, H, M;

    r = call_conf_default("TODAY", buffer, NULL);
    ck_assert_int_eq(r, 0);

    r = sscanf(buffer, "%d-%[^-]-%d %d:%d (%[^)])", &d, m, &y, &H, &M, z);
    ck_assert_int_eq(r, 6);
    ck_assert_int_ge(d, 1);
    ck_assert_int_le(d, 31);
    ck_assert_int_ge(y, 2020);
    ck_assert_int_ge(H, 0);
    ck_assert_int_le(H, 24);
    ck_assert_int_ge(M, 0);
    ck_assert_int_le(M, 59);
}
#endif
/*********************************************************************/
/*********************************************************************

TEST CASE "expand"


These tests are checking functions of 'redconf-expand.c'.

*********************************************************************/
/*********************************************************************/

#define KEY_PREFIX         "NODE_PREFIX"
#define KEY_MAIN           "redpak_MAIN"
#define KEY_CONF           "REDNODE_CONF"
#define KEY_NODE_ALIAS     "NODE_ALIAS"
#define KEY_NODE_NAME      "NODE_NAME"
#define KEY_NODE_PATH      "NODE_PATH"
#define KEY_NODE_INFO      "NODE_INFO"

#define VAL_NODE_ALIAS     "alias"
#define VAL_NODE_NAME      "###name of $NODE_ALIAS###"
#define VAL_NODE_PATH      "###ROOT/$NODE_ALIAS/here###"
#define VAL_NODE_INFO      "###info of $NODE_NAME###"

#define VAL_PREFIX         "(((PREFIX)))"

#define EXP_PREFIX         VAL_PREFIX
#define EXP_MAIN           VAL_PREFIX redpak_MAIN
#define EXP_NODE_ALIAS     VAL_NODE_ALIAS
#define EXP_NODE_NAME      "###name of "EXP_NODE_ALIAS"###"
#define EXP_NODE_PATH      "###ROOT/"EXP_NODE_ALIAS"/here###"
#define EXP_NODE_INFO      "###info of "EXP_NODE_NAME"###"
#define EXP_CONF           EXP_NODE_PATH"/"REDNODE_CONF

void test_exp_def(const char *key, const char *value, const redNodeT *node)
{
    static const char prefix[] = "<<<";
    static const char suffix[] = ">>>";

    char tbe_key[1000], tbe_val[1000], scratch[1000];
    char *str;
    int rc, len, len2;

    str = RedGetDefaultExpand(node, key);
    printf("%s -> %s\n", key, str);
    ck_assert_ptr_nonnull(str);
    ck_assert_str_eq(str, value);
    free(str);

    sprintf(tbe_key, "{$%s}.{$%s}", key, key);
    sprintf(tbe_val, "{%s}.{%s}", value, value);

    str = RedNodeStringExpand(node, tbe_key);
    printf("%s -> %s\n", tbe_key, str);
    ck_assert_ptr_nonnull(str);
    ck_assert_str_eq(str, tbe_val);
    free(str);

    sprintf(tbe_key, "{$%s}", key);
    sprintf(tbe_val, "%s{%s}%s", prefix, value, suffix);

    len = 0;
    rc = RedConfAppendEnvKey(node, scratch, &len, 1000, tbe_key, prefix, suffix);
    printf("%s -> %s\n", tbe_key, scratch);
    ck_assert_int_eq(rc, 0);
    ck_assert_int_eq(len, strlen(scratch));
    ck_assert_str_eq(scratch, tbe_val);

    len2 = 0;
    rc = RedConfAppendEnvKey(node, scratch, &len2, len, tbe_key, prefix, suffix);
    ck_assert_int_ne(rc, 0);
}

void test_exp_def_env(const char *key, const char *value, const redNodeT *node)
{
    static const char envval[] = "+value+";
    char *prv = getenv(key);

    test_exp_def(key, value, node);
    if (prv != NULL)
        prv = strdup(prv);
    setenv(key, envval, 1);
    test_exp_def(key, envval, node);
    if (prv == NULL)
        unsetenv(key);
    else {
        setenv(key, prv, 1);
        free(prv);
    }
}

/* basic expansion test */
START_TEST(test_expand)
{
    redNodeT node;
    redConfigT config;
    redConfHeaderT header;

    memset(&node, 0, sizeof node);
    memset(&config, 0, sizeof config);
    node.config = &config;
    config.headers = &header;
    node.leaf = &node;
    node.redpath = VAL_NODE_PATH;
    header.alias = VAL_NODE_ALIAS;
    header.name = VAL_NODE_NAME;
    header.info = VAL_NODE_INFO;


    setenv(KEY_PREFIX, VAL_PREFIX, 1);

    test_exp_def_env(KEY_PREFIX, VAL_PREFIX, &node);

    setenv(KEY_PREFIX, VAL_PREFIX, 1);

    test_exp_def_env(KEY_MAIN, EXP_MAIN, &node);
    test_exp_def(KEY_NODE_ALIAS, EXP_NODE_ALIAS, &node);
    test_exp_def(KEY_NODE_NAME, EXP_NODE_NAME, &node);
    test_exp_def(KEY_NODE_PATH, EXP_NODE_PATH, &node);
    test_exp_def(KEY_NODE_INFO, EXP_NODE_INFO, &node);
    test_exp_def_env(KEY_CONF, EXP_CONF, &node);
}

START_TEST(test_expand_cmd)
{
    char today[100];
    char pattern[100];
    char input[100];
    char *output;
    int rc;

    /* get todays date and write it in temporary file */
    rc = getDateOfToday(today, sizeof today);
    ck_assert_int_ge(rc, 1);
    write_tempfile(today, (size_t)rc);

    /* create the pattern */
    rc = snprintf(pattern, sizeof pattern, "****%s****", today);
    ck_assert_int_ge(rc, 0);
    ck_assert_int_lt(rc, (int)sizeof pattern);

    /* create the input string to expand */
    rc = snprintf(input, sizeof input, "****$(cat %s)****", tempname);
    ck_assert_int_ge(rc, 0);
    ck_assert_int_lt(rc, (int)sizeof input);

    /* check not expanding */
    output = expandAlloc(NULL, input, 0);
    ck_assert_ptr_nonnull(output);
    ck_assert_str_ne(output, pattern);
    ck_assert_str_eq(output, input);
    free(output);

    /* check expanding */
    output = expandAlloc(NULL, input, 1);
    ck_assert_ptr_nonnull(output);
    ck_assert_str_ne(output, input);
    ck_assert_str_eq(output, pattern);
    free(output);
}

/*********************************************************************/
/*********************************************************************

TEST CASE "schema"


These tests are checking functions of 'redconf-schema.c'.

*********************************************************************/
/*********************************************************************/

/* Helper function for testing functions
   RedLoadConfig, RedDumpConfigHandle, RedSaveConfig, RedGetConfigYAML and RedFreeConfig
*/
static void do_test_config(const char *path, int exists)
{
    int         rc;
    redConfigT *config, *other_config;
    char       *text,   *other_text;
    size_t      length, other_length;

    config = RedLoadConfig(path, 1);
    if (!exists)
        ck_assert_ptr_null(config);
    else {
        ck_assert_ptr_nonnull(config);

        /* dump the config */
        printf("\n\n\n**************** DUMP OF %s\n", path);
        RedDumpConfigHandle(config);

        /* get yaml of the config */
        rc = RedGetConfigYAML(&text, &length, config);
        ck_assert_int_eq(rc, 0);
        printf("\n\n\n**************** YAML OF %s\n%.*s\n", path, (int)length, text);

        /* write the yaml */
        write_tempfile(text, length);

        /* read the written yaml */
        other_config = RedLoadConfig(tempname, 1);
        ck_assert_ptr_nonnull(config);

        /* get yaml of the new read config */
        rc = RedGetConfigYAML(&other_text, &other_length, other_config);
        ck_assert_int_eq(rc, 0);

        /* compare the two yamls */
        ck_assert(other_length == length);
        ck_assert(0 == memcmp(other_text, text, length));

        free(other_text);
        RedFreeConfig(other_config, 1);
        free(text);
        RedFreeConfig(config, 1);
    }
}

/* Test that RedLoadConfig works by loading the current templates */
START_TEST(test_config)
{
    do_test_config(TEMPLATES_DIR "/" "admin-no-system-node.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "admin.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "default-no-system-node.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "default.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "main-admin-system.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "main-system.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "main-system.yaml", 1);
    do_test_config(TEMPLATES_DIR "/" "i-should-not-exists.yaml", 0);
}

typedef struct { const char *str; unsigned val; } assoc_string_uint_t;

const assoc_string_uint_t assoc_export[] = {
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

const assoc_string_uint_t assoc_varenv[] = {
    {"Static",   RED_CONFVAR_STATIC},
    {"Execfd",   RED_CONFVAR_EXECFD},
    {"Default",  RED_CONFVAR_DEFLT},
    {"Remove",   RED_CONFVAR_REMOVE},
};

const assoc_string_uint_t assoc_confopt[] = {
   {"Unset"  , RED_CONF_OPT_UNSET},
   {"Enabled", RED_CONF_OPT_ENABLED},
   {"Disabled",RED_CONF_OPT_DISABLED},
};

const assoc_string_uint_t assoc_status[] = {
    { "Disable", RED_STATUS_DISABLE},
    { "Enable",  RED_STATUS_ENABLE},
    { "Unknown", RED_STATUS_UNKNOWN},
    { "Error",   RED_STATUS_ERROR},
};

typedef const char *(*get_str_f)(unsigned);
const struct {
    get_str_f function;
    const assoc_string_uint_t *assoc;
    unsigned count;
    }
        assoc_testers[] =
    {
#define ITEM(fun, def) { (get_str_f)fun, def, (unsigned)(sizeof def / sizeof *def) }
        ITEM(getExportFlagString, assoc_export),
        ITEM(getRedVarEnvString, assoc_varenv),
        ITEM(getRedConfOptString, assoc_confopt),
        ITEM(getStatusFlagString, assoc_status)
#undef ITEM
    };


START_TEST(test_schema_string)
{
    const assoc_string_uint_t *iter, *end;
    get_str_f fun;
    unsigned isec;
    const unsigned nsec = (unsigned)(sizeof assoc_testers / sizeof *assoc_testers);

    for (isec = 0 ; isec < nsec ; isec++) {
        fun = assoc_testers[isec].function;
        iter = assoc_testers[isec].assoc;
        end = &iter[assoc_testers[isec].count];
        for( ; iter != end ; iter++)
            ck_assert_str_eq(iter->str, fun(iter->val));
    }
}

static const char *statusFlagStrings[] = {
    [RED_STATUS_DISABLE] = "Disable",
    [RED_STATUS_ENABLE] = "Enable",
    [RED_STATUS_UNKNOWN] = "Unknown",
    [RED_STATUS_ERROR] = "Error"
};


void fwrite_status(const char *path, const char *info, const char *timestamp, const char *state, FILE *file)
{
    if (path)
        fprintf(file, "realpath: %s\n", path);
    if (info)
        fprintf(file, "info: %s\n", info);
    if (timestamp)
        fprintf(file, "timestamp: %s\n", timestamp);
    if (state)
        fprintf(file, "state: %s\n", state);
}

void write_status(const char *path, const char *info, const char *timestamp, const char *state)
{
    FILE *file = fopen(tempname, "w");
    ck_assert_ptr_nonnull(file);
    fwrite_status(path, info, timestamp, state, file);
    fclose(file);
    printf("----\n");
    fwrite_status(path, info, timestamp, state, stdout);
}

void do_test_status(int ok, const char *path, const char *info, const char *timestamp, const char *state)
{
    int rc;
    redStatusT * status;
    write_status(path, info, timestamp, state);
    status = RedLoadStatus(tempname, 0);
    if (!ok)
        ck_assert_ptr_null(status);
    else {
        ck_assert_ptr_nonnull(status);
        ck_assert_str_eq(status->realpath, path);
        ck_assert_str_eq(statusFlagStrings[status->state], state);
        ck_assert_int_eq(status->timestamp, atol(timestamp));
        if (info)
            ck_assert_str_eq(status->info, info);

        rc = RedSaveStatus(tempname, status, 0);
        ck_assert_int_eq(rc, 0);

        RedFreeStatus(status, 0);

        status = RedLoadStatus(tempname, 0);
        ck_assert_ptr_nonnull(status);
        ck_assert_str_eq(status->realpath, path);
        ck_assert_str_eq(statusFlagStrings[status->state], state);
        ck_assert_int_eq(status->timestamp, atol(timestamp));
        if (info)
            ck_assert_str_eq(status->info, info);

        RedFreeStatus(status, 0);
    }
}

START_TEST(test_status)
{
    do_test_status(1, "/a/path", "some info", "123456789", statusFlagStrings[RED_STATUS_ENABLE]);
    do_test_status(1, "/a/path", NULL, "123456789", statusFlagStrings[RED_STATUS_ENABLE]);
    do_test_status(0, NULL, NULL, "123456789", statusFlagStrings[RED_STATUS_ENABLE]);
    do_test_status(0, "/a/path", NULL, NULL, statusFlagStrings[RED_STATUS_ENABLE]);
    do_test_status(0, "/a/path", NULL, "123456789", NULL);
    do_test_status(0, "/a/path", NULL, "alpha", statusFlagStrings[RED_STATUS_ENABLE]);
}

/*********************************************************************/
/*********************************************************************

TEST CASE "factory"


These tests are checking functions of 'rednode-factory.c'.

*********************************************************************/
/*********************************************************************/

static char bigname[REDNODE_FACTORY_PATH_LEN + 20];

static void init_bigname()
{
    memset(bigname, 'x', sizeof bigname - 1);
    bigname[sizeof bigname - 1] = 0;
}

#define BIGNAME(len)      (&bigname[((len) < (sizeof bigname)) ? ((sizeof bigname) - (len)) : 0])

#undef ROOT
#define ROOT "/tmp/rednode-root"
const
struct factorydef
{
    const char *root;
    const char *node;
    const char *alias;
    const char *normal;
    const char *admin;
    int update;
    int issys;
    rednode_factory_error_t expected;
}
    factories[] = 
{
#define TEST(root,node,alias,normal,admin,update,issys,expected) { root,node,alias,normal,admin,update,issys,expected }
#define MKDIR(path) { NULL,NULL,NULL,path,NULL,0,0,0 }
#define RMDIR(path) { NULL,NULL,NULL,NULL,path,0,0,0 }

    RMDIR(ROOT),
    TEST(BIGNAME(REDNODE_FACTORY_PATH_LEN), NULL, NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Root_Too_Long),
    TEST("simple", NULL, NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Root_Not_Absolute),
    TEST("/tmp", BIGNAME(REDNODE_FACTORY_PATH_LEN), NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Node_Too_Long),
    TEST(NULL, "simple", NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Cleared),
    TEST(NULL, NULL, "simple", NULL, NULL, 0, 0, RednodeFactory_Error_Cleared),
    TEST("/tmp", NULL, NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Default_Alias_Empty),
    TEST("/tmp", NULL, "alias", BIGNAME(REDNODE_FACTORY_PATH_LEN), NULL, 0, 0, RednodeFactory_Error_Config_Too_Long),
    TEST("/tmp", NULL, "alias", "not-existing", NULL, 0, 0, RednodeFactory_Error_No_Config),
    TEST("/tmp", NULL, "alias", NULL, "not-existing", 0, 0, RednodeFactory_Error_No_Config),
    TEST("/tmp", NULL, "alias", tempname, NULL, 0, 0, RednodeFactory_Error_Loading_Config),
    TEST("/tmp", NULL, "alias", NULL, tempname, 0, 0, RednodeFactory_Error_Loading_Config),
    TEST(ROOT, "simple", NULL, NULL, NULL, 0, 0, RednodeFactory_Error_Root_Not_Exist),
    MKDIR(ROOT),
    TEST(ROOT, "simple", NULL, NULL, NULL, 0, 0, RednodeFactory_OK),
    TEST(ROOT, "simple/subsimple", NULL, NULL, NULL, 0, 0, RednodeFactory_OK),
    RMDIR(ROOT),

#undef TEST
#undef MKDIR
#undef RMDIR
};


START_TEST(test_factory)
{
    int rc;
    const struct factorydef *iter = factories;
    const struct factorydef *end = &factories[sizeof factories / sizeof *factories];

    init_bigname();
    write_tempfile("$%!?##\n", 0);
    setenv("REDNODE_TEMPLATE_DIR", TEMPLATES_DIR, 1);
    for (; iter != end ; iter++) {
        if (iter->root != NULL || iter->node != NULL) {
            rednode_factory_t factory;
            rednode_factory_param_t params, *ppar;
            rednode_factory_error_t rfs;

            printf("\nchecking factory\n");
            printf("    - root   %s\n", iter->root);
            printf("    - node   %s\n", iter->node);
            printf("    - alias  %s\n", iter->alias);
            printf("    - normal %s\n", iter->normal);
            printf("    - admin  %s\n", iter->admin);
            printf("    - update %d\n", iter->update);
            printf("    - issys  %d\n", iter->issys);
            rfs = RednodeFactory_OK;
            rednode_factory_clear(&factory);
            if (iter->root != NULL)
                rfs = rednode_factory_set_root(&factory, iter->root);
            if (rfs == RednodeFactory_OK && iter->node != NULL)
                rfs = rednode_factory_set_node(&factory, iter->node);
            if (rfs == RednodeFactory_OK) {
                if (iter->alias == NULL && iter->normal == NULL && iter->admin == NULL)
                    ppar = NULL;
                else {
                    params.alias = iter->alias;
                    params.normal = iter->normal;
                    params.admin = iter->admin;
                    ppar = &params;
                }
                rfs = rednode_factory_create_node(&factory, ppar, iter->update, iter->issys);
            }
            printf("    - expec  %d\n", iter->expected);
            printf("    = got    %d\n", -rfs);
            ck_assert_int_eq(-rfs, iter->expected);
        }
        else if (iter->normal != NULL) {
            printf("\nchecking factory, create dir %s\n", iter->normal);
            rc = mkdir(iter->normal, 0);
            ck_assert_int_eq(rc, 0);
        }
        else if (iter->admin != NULL) {
            printf("\nchecking factory, remove dir %s\n", iter->admin);
            remove_directories(iter->admin);
        }
    }
}

/*********************************************************************/


static Suite *suite;
static TCase *tcase;

void mksuite(const char *name) { suite = suite_create(name); }
void addtcase(const char *name) { tcase = tcase_create(name); suite_add_tcase(suite, tcase); }
void addtcasefix(const char *name, SFun setup, SFun teardown) { addtcase(name); tcase_add_checked_fixture(tcase, setup, teardown); }

#define addtest(test) tcase_add_test(tcase, test)
int srun()
{
	int nerr;
	SRunner *srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	nerr = srunner_ntests_failed(srunner);
	srunner_free(srunner);
	return nerr;
}

int main(int ac, char **av)
{
	mksuite("checks-conf");
#if 0
        addtcase("defaults");
            addtest(test_defaults_env);
            addtest(test_defaults_int);
            addtest(test_defaults_for_node);
            addtest(test_defaults_uuid);
            addtest(test_defaults_today);
#endif
        addtcasefix("expand", make_tempname, remove_tempfile);
            addtest(test_expand);
            addtest(test_expand_cmd);
        addtcasefix("schema", make_tempname, remove_tempfile);
            addtest(test_config);
            addtest(test_schema_string);
            //TODO: addtest(test_config_validation);
            addtest(test_status);
        addtcasefix("factory", make_tempname, remove_tempfile);
            addtest(test_factory);
	return !!srun();
}



