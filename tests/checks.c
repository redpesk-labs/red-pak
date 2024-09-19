#include <check.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "redconf-utils.h"
#include "redwrap-exec.h"
#include "redconf-merge.h"

#ifndef REDMICRODNF_CMD
#define REDMICRODNF_CMD "/usr/bin/redmicrodnf"
#endif

#ifndef TEMPLATES_DIR
#define TEMPLATES_DIR "."
#endif

#define BWRAP "/usr/bin/bwrap"

#define ROOT_REDPATH "/tmp/redpaktest"

#define PARENT_REDPATH ROOT_REDPATH"/systemnode/parent"
#define CHILD_REDPATH PARENT_REDPATH"/child"

#define PARENT_REDPATH_NOS ROOT_REDPATH"/nosystemnode/parent"
#define CHILD_REDPATH_NOS PARENT_REDPATH_NOS"/child"

#define PARENT_REDPATH_TEMPLATES ROOT_REDPATH"/templates/parent"
#define CHILD_REDPATH_TEMPLATES PARENT_REDPATH_TEMPLATES"/child"
#define TEMPLATE TEMPLATES_DIR"/default.yaml"
#define TEMPLATE_ADMIN TEMPLATES_DIR"/admin.yaml"

static void debugargv(char * const argv[]) {
    int i = 0;
    printf("[CMD] = ");
    while(argv[i]) {
        printf("%s ", argv[i]);
        i++;
    }
    printf("\n");
}

/* create node */
static int redmicrodnf_manager(char * const argv[]) {
    debugargv(argv);
    int pid = fork();
    if (pid == 0) {
        int error;
        error = execv(REDMICRODNF_CMD, argv);
        if(error)
            fprintf(stderr, "Issue exec outnode command %s error:%s\n", REDMICRODNF_CMD, strerror(errno));
    }
    int returnStatus;
    waitpid(pid, &returnStatus, 0);
    return returnStatus;
}

static int createNodeNoSystem(const char *redpath) {
    char * argv[] = {
        REDMICRODNF_CMD,
        "--redpath",
        (char *)redpath,
        "manager",
        "--no-system-node",
        NULL
    };

    return redmicrodnf_manager(argv);
}

static int createNodeWithSystem(char *const redpath) {
    char *const argv[] = {
        "redmicrodnf",
        "--redpath",
        redpath,
        "manager",
        NULL
    };

    return redmicrodnf_manager(argv);
}

static int createNodeWithTemplates(char *const redpath, char *const tpl, char *const tpladmin) {
    char *const argv[] = {
        "redmicrodnf",
        "--redpath",
        redpath,
        "manager",
        "--template",
        tpl,
        "--template-admin",
        tpladmin,
        NULL
    };

    return redmicrodnf_manager(argv);
}

static int testcmd(const char *redpath, char *cmd) {
    int subargc = 1;
    char *subargv[] = {
        cmd,
        "NULL"
        };

    rWrapConfigT cliargs = {
        .redpath = redpath,
        .bwrap = BWRAP,
        .adminpath = NULL,
        .index = 0,
        .verbose = 1,
        .strict = 0,
        .unsafe = 0
    };

    printf("\n[TESTCMD] = redpath=%s cmd=%s", redpath, cmd);
    return redwrapExecBwrap(cmd, &cliargs, subargc, subargv);
}

static void test_simple_cmds(const char *redpath) {
    ck_assert_int_eq(testcmd(redpath, "ls"), 0);
    ck_assert_int_eq(testcmd(redpath, "sureititdoesnotexits"), 256);
}

static void test_config(const char *redpath) {
    printf("[TEST CONFIG] ...");
    size_t len;
    const char *resyaml = getConfig(redpath, &len);
    ck_assert_ptr_nonnull(resyaml);
}

static void test_mergeconfig(const char *redpath) {
    size_t len;
    const char *resyaml = getMergeConfig(redpath, &len, 0);
    ck_assert_ptr_nonnull(resyaml);
}

static void test_node(const char *redpath) {
    test_simple_cmds(redpath);
    test_config(redpath);
    test_mergeconfig(redpath);
}

START_TEST (test_node_parent) {
    test_node(PARENT_REDPATH);
}
END_TEST

START_TEST (test_node_child) {
    test_node(CHILD_REDPATH);
}
END_TEST

START_TEST (test_node_parent_nos) {
    test_node(PARENT_REDPATH_NOS);
}
END_TEST

START_TEST (test_node_child_nos) {
    test_node(CHILD_REDPATH_NOS);
}
END_TEST

START_TEST (test_node_parent_templates) {
    test_node(PARENT_REDPATH_TEMPLATES);
}
END_TEST

START_TEST (test_node_child_templates) {
    test_node(CHILD_REDPATH_TEMPLATES);
}
END_TEST

static void tc_system_node_setup(void) {
    printf("\n\n *** Setup nodes: %s\n", PARENT_REDPATH);
    remove_directories(PARENT_REDPATH);
    ck_assert_int_eq(createNodeWithSystem(PARENT_REDPATH), 0);
    ck_assert_int_eq(createNodeWithSystem(CHILD_REDPATH), 0);
}

static void tc_system_node_teardown(void) {
}

static void tc_no_system_node_setup(void) {
    printf("\n\n *** Setup nosystem nodes: %s\n", PARENT_REDPATH_NOS);
    remove_directories(PARENT_REDPATH_NOS);
    ck_assert_int_eq(createNodeNoSystem(PARENT_REDPATH_NOS), 0);
    ck_assert_int_eq(createNodeNoSystem(CHILD_REDPATH_NOS), 0);
}

static void tc_no_system_node_teardown(void) {
}

static void tc_templates_setup(void) {
    printf("\n\n *** Setup templates nodes: %s with template=%s template admin=%s\n",
        PARENT_REDPATH_TEMPLATES, TEMPLATE, TEMPLATE_ADMIN);
    remove_directories(PARENT_REDPATH_TEMPLATES);
    ck_assert_int_eq(createNodeWithTemplates(PARENT_REDPATH_TEMPLATES, TEMPLATE, TEMPLATE_ADMIN), 0);
    ck_assert_int_eq(createNodeWithTemplates(CHILD_REDPATH_TEMPLATES, TEMPLATE, TEMPLATE_ADMIN), 0);
}

static void tc_templates_teardown(void) {
}

Suite * tests_suite(void) {
    Suite *s;
    TCase *tc_system_node;
    TCase *tc_no_system_node;
    TCase *tc_templates_node;

    s = suite_create("********** Redpak Suite Test ***********\n\n");

    /*************************************
    *** Creation test case: system node */
    tc_system_node = tcase_create("\n\n************* Test System Node *************");
    suite_add_tcase(s, tc_system_node);
    tcase_add_checked_fixture(tc_system_node, tc_system_node_setup, tc_system_node_teardown);

    tcase_add_test(tc_system_node, test_node_parent);
    tcase_add_test(tc_system_node, test_node_child);

    /****************************************
    *** Creation test case: no system node */
    tc_no_system_node = tcase_create("\n\n************* Test No System Node *************");
    suite_add_tcase(s, tc_no_system_node);
    tcase_add_checked_fixture(tc_no_system_node, tc_no_system_node_setup, tc_no_system_node_teardown);

    tcase_add_test(tc_no_system_node, test_node_parent_nos);
    tcase_add_test(tc_no_system_node, test_node_child_nos);

    /****************************************
    *** Creation test case: templates*/
    tc_templates_node = tcase_create("\n\n************* Test Templates *************");
    suite_add_tcase(s, tc_templates_node);
    tcase_add_checked_fixture(tc_templates_node, tc_templates_setup, tc_templates_teardown);

    tcase_add_test(tc_templates_node, test_node_parent_templates);
    tcase_add_test(tc_templates_node, test_node_child_templates);

    return s;
}

int main(void)
{
    printf("REDMICRODNF_CMD=%s\n", REDMICRODNF_CMD);
    int number_failed;
    SRunner *sr;

    sr = srunner_create(tests_suite());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_set_log (sr, "test.log");
    srunner_set_xml (sr, "test.xml");
    srunner_run_all(sr, CK_VERBOSE);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
