#include <string.h>

#include "utils.h"

void usageOptions(const rOption *options) {
    printf("\nOptions:\n");
    for (const rOption *opt = options; opt->option.name; opt++) {
        printf("\t-%c, --%s\t\t%s\n", opt->option.val, opt->option.name, opt->desc);
    }
}

/* getLongOpt: copy struct option into struct option array for getopt_long*/
void setLongOptions(const rOption opts[], struct option *longOpts) {
    struct option *tmpLongOpt = longOpts;
    for (rOption *opt = (rOption *)opts; opt->option.name; opt++) {
        memcpy(tmpLongOpt, opt, sizeof(struct option));
        tmpLongOpt++;
    }
    memset(tmpLongOpt, 0, sizeof(struct option));
}
