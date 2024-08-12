#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "redconf-defaults.h"
#include "redconf-schema.h"



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
	return !!srun();
}



