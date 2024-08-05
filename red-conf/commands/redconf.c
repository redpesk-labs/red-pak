#include <string.h>

#include "utils.h"
#include "../redconf-defaults.h"

//include subcommands
#include "config.h"
#include "mergeconfig.h"
#include "tree.h"

typedef struct {
    const char *cmd_name;
    int (*cmd)(const rGlobalConfigT *);
    const char *desc;
} rCommandT;

/***************************************
 **** Global command ****
 **************************************/

//all subcommand of redconf
static const rCommandT commands[] = {
    {"config", config, "get configuration files from nodes"},
    {"mergeconfig", mergeconfig, "get merge configuration files from nodes"},
    {"tree", tree, "get tree nodes"},
    {0, 0, 0}
};

const RedLogLevelE verbose_default = REDLOG_WARNING;

static const rOption globalOptions[] = {
    {{"verbose",    optional_argument, (int *)&verbose_default, 'v'}, "increase verbosity to info"},
    {{"yaml",       no_argument      ,     0,                   'y'}, "yaml output"},
    {{"log-yaml",   required_argument,     0,                   'l'}, "yaml parse log level (max=4)"},
    {{"help",       no_argument      ,     0,                   'h'}, "print this help"},
    {{0, 0, 0}, 0}
};

static const char SHORTOPTS[] = "v::hyl:";

static void globalUsage(const rOption *options) {
    printf("Usage: redconf [OPTION]... [COMMAND]... [OPTION]...\n"
           "redpak configuration commands\n"
           "\n"
           "Commands:\n"
    );
    for (const rCommandT *cmd = commands; cmd->cmd; cmd++)
        printf("\t%s\t\t%s\n", cmd->cmd_name, cmd->desc);

    usageOptions(options);
    exit(1);
}

static int globalParseArgs(int argc, char *argv[], rGlobalConfigT *gConfig) {
    int index = 1;
    int option;
    struct option longOpts [sizeof(struct option) * sizeof(globalOptions) / sizeof(globalOptions[0])];
    setLongOptions(globalOptions, longOpts);

    while(1) {
        //is it a command ?: if break
        printf("index(=%d) < argc(=%d) && argv[index][0](=%s)\n", index, argc, argv[index]);
        if (index < argc && argv[index][0] != '-') {
            gConfig->cmd = argv[index];
            gConfig->sub_argc = argc - index;
            gConfig->sub_argv = argv + index;

            //reset optind
            optind = 1;
            break;
        }

           option = getopt_long(argc, argv, SHORTOPTS, longOpts, NULL);
        if (option == -1) //no more options
               break;

        RedLogLevelE verbosity = REDLOG_INFO;
        printf("OPTOPT %c\n", optopt);
        // option return short option even when long option is given
          switch (option) {
              case 'v':
                if(optarg) {
                    size_t length = strlen(optarg);
                    if (!strncmp(optarg, "v", length)) {
                        verbosity = REDLOG_DEBUG;
                    } else if (!strncmp(optarg, "vv", length)) {
                        verbosity = REDLOG_TRACE;
                    } else {
                        optarg = NULL;
                    }
                }
                SetLogLevel(verbosity);
                gConfig->verbose = 1;
                  break;

            case 'y':
                gConfig->yaml = 1;
                break;

            case 'l':
                setLogYaml(atoi(optarg));
                break;

            case 'h': default:
                globalUsage(globalOptions);
                break;
            case '?':
                goto OnErrorExit;
        }

        //get next index command line
        index = optind;
        printf("OPTIND %d argv[optind]=%s optarg=%s\n", optind, argv[optind], optarg);
        if(optarg) {
            //if next arg = argv[optind] then bypass
            if(!strncmp(optarg, argv[optind], strlen(optarg)))
                index++;
        }

    }
    return 0;

OnErrorExit:
    return -1;
}

int main (int argc, char *argv[]) {
    rGlobalConfigT gConfig = {0};
    if (globalParseArgs(argc, argv, &gConfig) < 0)
        goto OnErrorExit;

    //if no sub command issue
    if(!gConfig.cmd) {
        RedLog(REDLOG_ERROR, "No command options found\n");
        globalUsage(globalOptions);
        goto OnErrorExit;
    }

    //call right sub command
    for(const rCommandT *cmd = commands; cmd->cmd_name; cmd++) {
        if (!strcmp(cmd->cmd_name, gConfig.cmd)) {
            return cmd->cmd(&gConfig);
        }
    }

OnErrorExit:
    return -1;

}