#ifndef _REDCONFCMD_UTILS_INCLUDE_
#define _REDCONFCMD_UTILS_INCLUDE_

#include <getopt.h>
#include "../redconf.h"

typedef struct {
	const char* cmd;
    int sub_argc;
	char **sub_argv;
    int verbose;
	int yaml;
	int logyaml;
} rGlobalConfigT;

typedef struct {
	struct option option;
	const char *desc;
} rOption;

void usageOptions(const rOption *options);
void setLongOptions(const rOption opts[], struct option *longOpts);

#endif