#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "redconf-defaults.h"
#include "redconf-schema.h"
#include "redconf-node.h"
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

/*********************************************************************/
/*********************************************************************

TEST CASE "defaults"


These tests are checking that the expansions of predefined variables
is correct and that the their default values are also correct.

The name default comes from 'redconf-defaults.c'.

*********************************************************************/
/*********************************************************************/

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


/* TEST expansion of of variable with (or not) default values */
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
    FILE       *file;
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
        file = fopen(tempname, "w");
        ck_assert_ptr_nonnull(file);
        rc = (int)fwrite(text, length, 1, file);
        ck_assert_int_eq(rc, 1);
        fclose(file);

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
		addtcase("defaults");
			addtest(test_defaults_env);
			addtest(test_defaults_int);
                        addtest(test_defaults_for_node);
                        addtest(test_defaults_uuid);
                        addtest(test_defaults_today);
		addtcasefix("schema", make_tempname, remove_tempfile);
                        addtest(test_config);
                        //TODO: addtest(test_config_validation);
                        //TODO: addtest(test_status);
                        addtest(test_schema_string);
	return !!srun();
}



