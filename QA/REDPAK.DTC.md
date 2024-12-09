# Design test cases of redpesk-labs/red-pak

.VERSION: 2.3.x

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.REVIEW: IREV1

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as *REDPAK*.

This document list the tests ensuring conformance to design
of *REDPAK* (@REDPAK.DSG) are fulfilled.

## Integration tests

### Configuration tests

Tests checking correctness of configuration parsing.

.TYPE integration

#### Description

The configuration tests are checking that configuration files are
correctly handled.

The methodology is to create rednodes with tuned configuration
and to ask *REDCONF* to display that configuration.

The output of *REDCONF* is compared to a validated reference
for that given configuration.

The conf tests are located in directory `tests/conf/`. It contains:

- `do-conf-tests.sh`: script that run tests and emits results
- `data/`: directory containing test definitions
- `conf-tests.tap`: last recorded TAP output

In directory `tests/conf/data`, a file `X-input` is the
configuration file of **test-conf-X** and the file
`X-reference` is its expected output (after filtering
out of variable parts of the output that are marked with
the tag **`IGNORE`**).

In these tests, the references are outputs that were validated
as matching the expected requirements by the test officer.

So for each of the below tests, the procedure applied
by the script `tests/conf/do-conf-tests.sh` is the same.
What changes are the input and consequently the expected
outputs.

#### Detailled procedure

.PROCEDURE

1. Remove the directory `/tmp/rpak-conftest` and all its content
2. Create the empty *REDNODE* root directory `/tmp/rpak-conftest`
3. Create the directory `/tmp/rpak-conftest/etc`
4. Copy in directory `/tmp/rpak-conftest/etc` the configuration file
   that is the subject of the test as `/tmp/rpak-conftest/etc/redpak.yaml`,
   the normal configuration file. For the test number *N*, the standard
   configuration file is named `tests/conf/data/N-input`
5. Create the status file `/tmp/rpak-conftest/.rednode.yaml` making
   `/tmp/rpak-conftest` a valid *REDNODE* root directory
6. Invoke *REDCONF* in order to dump out as YAML the configuration
   of the *REDNODE* `/tmp/rpak-conftest` using the command:
   `redconf --yaml config "/tmp/rpak-conftest"`
7. Compare the YAML output result of the command on step 6 (expurged
   of the lines marked with *IGNORE*) and the expected reference output
   of the file `tests/conf/data/N-reference`. The 2 files must be
   identical.


#### Invalid YAML is rejected

.TEST-CASE  test-conf-1

Invalid YAML file is rejected.

The configuration file `tests/conf/data/1-input` is an invalid
YAML file that must be rejected.

The reference file `tests/conf/data/1-reference` mainly contains
the error message.

.REQUIRED-BY @REDPAK.CNF-U-VAL-YAM-FIL

.REQUIRED-BY @REDPAK.DSG-R-RED-CON-FIL-ARE-YAM


#### Valid YAML is accepted

.TEST-CASE  test-conf-2

Valid YAML file is Accepted.

The configuration file `tests/conf/data/2-input` contains a valid
YAML file with all settings given. It must be accepted and processed.

The reference file `tests/conf/data/2-reference` is a dump of the input.

.REQUIRED-BY @REDPAK.DSG-R-RED-CON-FIL-ARE-YAM

.REQUIRED-BY @REDPAK.CNF-U-VAL-YAM-FIL

.REQUIRED-BY @REDPAK.CNF-U-VAL-ROO-MAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-HEA-MAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CGR-MAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CON-MAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-SEQ

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-SEQ


#### YAML keys are not case sensitive

.TEST-CASE  test-conf-3

The configuration parser ignore case when it process
keys or enumeration values.

The configuration file `tests/conf/data/3-input` contains
all settings with keys and values written with
characters in mixed upper and lower cases.

The reference file `tests/conf/data/3-reference` is a dump
of the input that is accepted as valid.

.REQUIRED-BY @REDPAK.CNF-U-KEY-ARE-NOT-CAS-SEN

#### Miss of required key is reported

.TEST-CASE  test-conf-4

The configuration parser detects that the key **header.alias**
misses and report it as an error.

The configuration file `tests/conf/data/4-input` contains
a valid YAML file but with some reqired entry missing.

The reference file `tests/conf/data/4-reference` is the
error report stated miss of items.

.REQUIRED-BY @REDPAK.CNF-U-CON-MUS-BE-VAL

.REQUIRED-BY @REDPAK.CNF-U-UNE-CON-NOT-VAL

#### Miss of optional parts is valid

.TEST-CASE  test-conf-5

A minimal configuration is given, it must be accepted.

The configuration file `tests/conf/data/5-input` contains
a valid YAML minimal.

The reference file `tests/conf/data/5-reference` contains
a basic dump of the input.

.REQUIRED-BY @REDPAK.CNF-U-CON-MUS-BE-VAL

#### Valid characters of alias

.TEST-CASE  test-conf-6

Valid characters of aliases must be accepted.

The configuration file `tests/conf/data/6-input` contains
a valid YAML minimal with the alias name containing all
the valid characters.

The reference file `tests/conf/data/6-reference` contains
a basic dump of the input.

.REQUIRED-BY @REDPAK.CNF-U-VAL-ALI

#### Invalid characters of alias

.TEST-CASE  test-conf-7

Invalid characters of aliases are rejected.

The configuration file `tests/conf/data/7-input` contains
a valid YAML minimal with the alias name containing
forbidden characters.

The reference file `tests/conf/data/6-reference` contains
the error report related to alias validity.

.REQUIRED-BY @REDPAK.CNF-U-VAL-ALI

### Basic tests

.TEST-CASE REDPAK.DTC-T-BAS-TES

Tests checking inheritance composition of basic settings.

.TYPE integration

#### Overview

These tests are checking options passed to bubble wrap.
They check:

- that configuration files are resulting in correct options
- that options resulting of merged configuration of layered *REDNODE*s
  is correct according to rules of composition

These tests are numerous and their expected results were validated
by a human. It can serve as non-regression test.

#### Methodology

For many pair of configuration's settings, including the null setting
(null setting is given by a configuration file having only the section
*headers*), five compositions *REDNODE*s are made and for each one the normal
and admin options are checked.

Let say the 2 settings are A and B. The 5 *REDNODE*s built for the pair are:

| case | Root Normal | Root Admin | Sub Normal | Sub Admin |
|------|-------------|------------|------------|-----------|
|  1   |    A        |    B       |     -      |    -      |
|  2   |    A        |   NULL     |     B      |   NULL    |
|  3   |    A        |   NULL     |    NULL    |    B      |
|  4   |   NULL      |    A       |     B      |   NULL    |
|  5   |   NULL      |    A       |    NULL    |    B      |

In the case 1, only one *REDNODE* is built for evaluation: the root *REDNODE*.

In other cases, the evaluated *REDNODE* is built on top of a root
*REDNODE*.

For each case, *REDWRAP* is called one time for normal mode and one time for admin
mode with the option `--dump-only` that only outputs the arguments of invocation of
bubblewrap.

- *case 1*: checks how normal and admin setting work in the same *REDNODE*
- *case 2*: checks how normal setting changes parent's normal setting
- *case 3*: checks how admin setting changes parent's normal setting
- *case 4*: checks how paren's admin setting changes normal setting
- *case 5*: checks how admin setting changes parent's admin setting


#### Description

The basic tests are located in directory `tests/basic/`. It contains:

- `do-basic-tests.sh`: script that run tests and emits results
- `gen-confs.sh`: script that generate all configuration files
- `references/`: directory containing expected results
- `basic-tests.tap`: last recorded TAP output

The test is run by the script `do-basic-tests.sh`.
It produces 3 artifact directories and a TAP output:

- `configs/`: directory containing the used configurations
- `rednodes/`: directory containing the created *REDNODE*s
- `results/`: directory containing the results of individuals tests

The results are files, each result is for one couple of settings and
contains the outputs of the 10 'pseudo' invocations of bubblewrap by *REDWRAP*.

The results and the references are named using the pattern `A__B` where A and B are
the settings as described above.

Currently available settings are:

| settings                        | settings                   |
|---------------------------------|----------------------------|
| config-cachedir                 | config-unsafe-false        |
| config-capa-cap_chown-0         | config-unsafe-true         |
| config-capa-cap_chown-1         | config-verbose-0           |
| config-capa-cap_setuid-0        | config-verbose-15          |
| config-capa-cap_setuid-1        | environ-default            |
| config-cgrouproot               | environ-execfd             |
| config-chdir                    | environ-remove             |
| config-die-with-parent-disabled | environ-static             |
| config-die-with-parent-enabled  | environ-unset              |
| config-die-with-parent-unset    | export-anonymous-A         |
| config-gpgcheck-false           | export-anonymous-B         |
| config-gpgcheck-true            | export-devfs-A             |
| config-hostname                 | export-devfs-B             |
| config-ldpath                   | export-execfd-A            |
| config-new-session-disabled     | export-execfd-B            |
| config-new-session-enabled      | export-internal-A          |
| config-new-session-unset        | export-internal-B          |
| config-path                     | export-lock-A              |
| config-persistdir               | export-lock-B              |
| config-rpmdir                   | export-mqueue-A            |
| config-share_all-disabled       | export-mqueue-B            |
| config-share_all-enabled        | export-private-A           |
| config-share_all-unset          | export-private-B           |
| config-share_cgroup-disabled    | export-privatefile-A       |
| config-share_cgroup-enabled     | export-privatefile-B       |
| config-share_cgroup-unset       | export-privaterestricted-A |
| config-share_ipc-disabled       | export-privaterestricted-B |
| config-share_ipc-enabled        | export-procfs-A            |
| config-share_ipc-unset          | export-procfs-B            |
| config-share_net-disabled       | export-public-A            |
| config-share_net-enabled        | export-public-B            |
| config-share_net-unset          | export-publicfile-A        |
| config-share_pid-disabled       | export-publicfile-B        |
| config-share_pid-enabled        | export-restricted-A        |
| config-share_pid-unset          | export-restricted-B        |
| config-share_time-disabled      | export-restrictedfile-A    |
| config-share_time-enabled       | export-restrictedfile-B    |
| config-share_time-unset         | export-symlink-A           |
| config-share_user-disabled      | export-symlink-B           |
| config-share_user-enabled       | export-tmpfs-A             |
| config-share_user-unset         | export-tmpfs-B             |
| config-umask-027                | NULL                       |
| config-umask-077                |                            |

See Appendice B for details about these basic configurations.

If fully implemented, the composition matrix would be huge: 7225
entries. But because many of these settings are unrelated one with
another, it is possible to only check the subset of pairs of settings
requiring application of merge rules.

This leads to a total of 293 meaningful pairs.

The 293 reference files are the outputs that were validated
as matching the expected requirements by the test officer.

See Appendice A for details about these tests.

#### Example of private export in hierarchy

.TEST-CASE export-private-A__export-private-A

.REQUIRED-BY @REDPAK.DSG-R-RED-LAY-HIE-FIL

#### Detailled procedure

.PROCEDURE

The test procedure takes 2 configurations: *CONF1* and *CONF2*.

1. Composition normal admin for same node

    a. create the rednode *node* with normal configuration
       *CONF1* and admin configuration *CONF2* using command
       `redconf create --alias NODE --config CONF1 --config-adm CONF2 node/CONF1/CONF2`

    b. dump the *bwrap* options of *node* in normal mode using command
       `redwrap --dump-only --redpath node/CONF1/CONF2`

    c. dump the *bwrap* options of *node* in admin mode using command
       `redwrap --admin --dump-only --redpath node/CONF1/CONF2`

2. Composition of normal subnode in normal node

    a. create the rednode *rootnn* with normal configuration
       *CONF1* and admin configuration *NULL* using command
       `redconf create --alias ROOT --config CONF1 --config-adm NULL rootnn/CONF1`

    b. create the rednode *subnn* with normal configuration
       *CONF2* and admin configuration *NULL* using command
       `redconf create --alias SUBNODE --config CONF2 --config-adm NULL rootnn/CONF1/CONF2`

    c. dump the *bwrap* options of *subnn* in normal mode using command
       `redwrap --dump-only --redpath rootnn/CONF1/CONF2`

    d. dump the *bwrap* options of *subnn* in admin mode using command
       `redwrap --admin --dump-only --redpath rootnn/CONF1/CONF2`

3. Composition of admin subnode in normal node

    a. create the rednode *rootna* with normal configuration
       *CONF1* and admin configuration *NULL* using command
       `redconf create --alias ROOT --config CONF1 --config-adm NULL rootna/CONF1`

    b. create the rednode *subna* with normal configuration
       *NULL* and admin configuration *CONF2* using command
       `redconf create --alias SUBNODE --config NULL --config-adm CONF2 rootna/CONF1/CONF2`

    c. dump the *bwrap* options of *subna* in normal mode using command
       `redwrap --dump-only --redpath rootna/CONF1/CONF2`

    d. dump the *bwrap* options of *subna* in admin mode using command
       `redwrap --admin --dump-only --redpath rootna/CONF1/CONF2`

4. Composition of normal subnode in admin node

    a. create the rednode *rootan* with normal configuration
       *NULL* and admin configuration *CONF1* using command
       `redconf create --alias ROOT --config NULL --config-adm CONF1 rootaa/CONF1`

    b. create the rednode *subnn* with normal configuration
       *CONF2* and admin configuration *NULL* using command
       `redconf create --alias SUBNODE --config CONF2 --config-adm NULL rootan/CONF1/CONF2`

    c. dump the *bwrap* options of *subnn* in normal mode using command
       `redwrap --dump-only --redpath rootan/CONF1/CONF2`

    d. dump the *bwrap* options of *subnn* in admin mode using command
       `redwrap --admin --dump-only --redpath rootan/CONF1/CONF2`

5. Composition of admin subnode in admin node

    a. create the rednode *rootaa* with normal configuration
       *NULL* and admin configuration *CONF1* using command
       `redconf create --alias ROOT --config NULL --config-adm CONF1 rootaa/CONF1`

    b. create the rednode *subna* with normal configuration
       *NULL* and admin configuration *CONF2* using command
       `redconf create --alias SUBNODE --config NULL --config-adm CONF2 rootaa/CONF1/CONF2`

    c. dump the *bwrap* options of *subna* in normal mode using command
       `redwrap --dump-only --redpath rootaa/CONF1/CONF2`

    d. dump the *bwrap* options of *subna* in admin mode using command
       `redwrap --admin --dump-only --redpath rootaa/CONF1/CONF2`

6. All the outputs and the commands are printed to the output file
   `results/CONF1__CONF2`

7. The output `results/CONF1__CONF2` is compared with the reference
   ouput `references/CONF1__CONF2` that has been validated by the
   test officer.

### Live tests

.TEST-CASE REDPAK.DTC-T-LIV-TES

Test that *REDWRAP* isolates correctly applications.

.TYPE integration

#### Description

Live tests are done by running a program in a given configuration.

Live test have number. The live test number N is achieved by applying
the below steps:

1. if the file `tests/live/data/N-normal.yaml` exists use it
   as NORMAL, the normal config file otherwise use `tests/live/data/0-normal.yaml`

2. if the file `tests/live/data/N-admin.yaml` exists use it
   as ADMIN, the admin config file, otherwise use `tests/live/data/0-admin.yaml`

3. create the test node in `/tmp/rpak-livetest/rnodN` of alias `NODE-N`
   using the command
   `redconf create --alias NODE-N --config NORMAL --config-adm ADMIN /tmp/rpak-livetest/rnodN`

4. if file `tests/live/data/N-prog.c` exists, compile it as
   `tests/live/data/N.prog` using command `cc -static -o tests/live/data/N.prog tests/live/data/N-prog.c`
   and set PROG to `tests/live/data/N.prog`, otherwise set PROG to `tests/live/data/N-prog`

5. if NORMAL references CMD, the file `/testscript/N-prog` (or `/testscript/N.prog` depending
   on PROG found in step 4), copy PROG to the location
   `/tmp/rpak-livetest/rnodN/testscript/N-prog` (or )`/tmp/rpak-livetest/rnodN/testscript/N.prog`
   depending on PROG found in step 4)

6. if NORMAL doesn't reference the file `/testscript/N-prog` (or `/testscript/N.prog` depending
   on PROG found in step 4), compute CMD as the busybox shell commands
   of `tests/live/data/N-prog` resulting of the command
   `sed 's/#.*//;/^[ \n]*$/d;s/.*/ & /' tests/live/data/N-prog | tr '\n' ';'`

7. execute CMD in the *REDNODE* `/tmp/rpak-livetest/rnodN`
   using command
   `redwrap --redpath /tmp/rpak-livetest/rnodN -- busybox sh -c "CMD"`
   and record the output of the command in `tests/live/data/N-result`
   after normalisation that replaces current process directory
   by `<<PWD>>`

8. compare `tests/live/data/N-result` purged from lines containing
   `IGNORE` with the references file `tests/live/data/N-reference`
   that was validated by the test officer as being the correct output

9. the name of the test is the first line of the file
   `tests/live/data/N-desc`

The live tests are located in directory `tests/live/`. It contains:

- `do-live-tests.sh`: script that run tests and emits results
- `data/`: directory containing test definitions
- `live-tests.tap`: last recorded TAP output

Definition of test **X** in `tests/conf/data` is using the
the files:

- `X-desc`: description of the test, the first line is the title
  of the test.

- `X-normal.yaml` and/or `X-admin.yaml`, the configuration of
  the tested *REDNODE*. When `X-normal.yaml` doesn't exists the
  file `0-normal.yaml` is used. When `X-admin.yaml` doesn't exists
  the file `0-admin.yaml` is used.

- `X-prog` or `X-prog.c` the program to execute in the node.
  When a C program is given, it is compiled statically using
  current C compiler: `cc`.

- `X-reference` the expected out put of the test (after filtering
  out of variable parts of the output).

#### REDNODEs are empty by default

.TEST-CASE  basically-nothing

Simple test entering a node with nothing except busybox at root.
Displays the content and the environment.

Setting: *tests/live/data/1-normal*

- export *busybox* at */*.

Program: *tests/live/data/1-prog*

- print filesystem content using `/busybox find /`
- print environment using `/busybox env`
- print current working directory using `/busybox pwd`

.REQUIRED-BY @REDPAK.HRQ-R-RED-ALW-RED-FIL-ROO

.REQUIRED-BY @REDPAK.HRQ-R-RED-SET-ISO-PRO

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM

#### Variables are set and expanded

.TEST-CASE setting-of-variables

Simple test entering a node with nothing except busybox at root.
Defines 4 environment variables.
Displays the content and the environment.

Setting: *tests/live/data/2-normal*

- export *busybox* at */*.
- define variables NOTEXPANDED, NOTEXPANDED2, EXPANDED, CMDOUTPUT
  using modes Static, Default, Default, Execfd respectively

Program: *tests/live/data/2-prog*

- print filesystem content using `/busybox find /`
- print environment using `/busybox env`
- print current working directory using `/busybox pwd`

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-ENV-ENT

.REQUIRED-BY @REDPAK.CNF-U-PAT-EXP-CHA

.REQUIRED-BY @REDPAK.CNF-U-ESC-PAT-EXP-CHA

.REQUIRED-BY @REDPAK.CNF-U-EXP-OCC-STR

.REQUIRED-BY @REDPAK.CNF-U-EXP-COM-EVA-REP

.REQUIRED-BY @REDPAK.CNF-U-CON-EXP

#### Files are mounted in created directory

.TEST-CASE path-to-mounted-bin

Simple test entering a node with nothing except busybox in /bin.
Defines the PATH environment variable and try to use it.
Displays the content and the environment.

Setting: *tests/live/data/3-normal*

- export *busybox* at */bin*
- define path as */bin*

Program: *tests/live/data/3-prog*

- print filesystem content using `busybox find /`
- print environment using `busybox env`
- print current working directory using `busybox pwd`
- print path of busybox using `busybox wich busybox`

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-ENT

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM

#### Can execute a script

.TEST-CASE execute-script

Simple test entering a node with busybox in /bin and program in testscript directory.
Defines the PATH environment variable and try to use it.
Displays the content and the environment.

Setting: *tests/live/data/4-normal*

- export *busybox* at */bin*
- export *4-prog* at */testscript*
- define path as */bin*

Program: *tests/live/data/4-prog*

- print filesystem content using `busybox find /`
- print environment using `busybox env`
- print current working directory using `busybox pwd`
- print path of busybox using `busybox wich busybox`

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-ENT

#### Variable expansion

.TEST-CASE expansion-of-keywords

Simple test entering a node with nothing except busybox at root.
Check the values of expanded variables.
Displays the environment.

Setting: *tests/live/data/5-normal*

- export *busybox* at */*
- define many environament variable as being expansion
  default of expanded variables and inherited using several
  modes

Program: *tests/live/data/5-prog*

- print environment using `/busybox env`

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-EXP-TAG-REP

.REQUIRED-BY @REDPAK.CNF-U-CON-EXP


#### Mounting directories, files, special file systems

.TEST-CASE basic-mounts

Simple test entering a node with busybox in /bin and program in testscript directory.
Mounts some basic export
Displays mounts and content of /bin

Setting: *tests/live/data/6-normal*

- export *busybox* at */bin*
- define path as */bin*
- export *6-prog* at */testscript*
- mount procfs, devfs, tmpfs, anonymous
- mount using symlink *sh*, *ls*, *sed*, *sort*, *echo* to */bin/busybox*

Program: *tests/live/data/6-prog*

- print content of */bin* using command `ls -l /bin/* | sed 's:.*[0-9] /bin:/bin:'`
- print mounted items using `sed 's:,.*::' /proc/mounts | sort`

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.HRQ-R-RED-MOU-SEL-PAR-HOS-FIL

.REQUIRED-BY @REDPAK.HRQ-R-RED-MOU-VOL-ARE-USI-TMP

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM


#### Memory coercion

.TEST-CASE mem-max

Check that setting config.cgroup.mem.max works

Setting: *tests/live/data/7-normal*

- export *busybox* at */bin*
- define path as */bin*
- export *7.prog* at */testscript*
- set *config.cgroups.mem.max* to 10000000

Program: *tests/live/data/7-prog.c*

- Allocates 1Mib, 2Mib, 4Mib, 8Mib, 16Mib, ... using `mmap`
  until allocation triggers an OOM kill of the process

.REQUIRED-BY @REDPAK.CNF-U-VAL-MEM-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR


#### Pid coercion

.TEST-CASE pids-max

Check that setting config.cgroup.pids.max works

Setting: *tests/live/data/8-normal*

- export *busybox* at */bin*
- define path as */bin*
- export *8.prog* at */testscript*
- set *config.cgroups.pids.max* to 15

Program: *tests/live/data/8-prog.c*

- fork it self while it is possible

.REQUIRED-BY @REDPAK.CNF-U-VAL-PID-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR

#### CPU coercion

.TEST-CASE cpu-max

Check that setting config.cgroup.cpu.max works

Setting: *tests/live/data/9-normal*

- export *busybox* at */bin*
- define path as */bin*
- export *9.prog* at */testscript*
- set *config.cgroups.cpu.max* to "150000 1000000"
  (meaning 15%)

Program: *tests/live/data/9-prog.c*

- during 10 seconds loop intensively, doing nothing
- after 10 seconds compute the load of the cpu the process got and print it

.REQUIRED-BY @REDPAK.CNF-U-VAL-CPU-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR

#### Capabilities settings

.TEST-CASE capabilities1

Check first half of capabilities

Setting: *tests/live/data/10-normal*

- export *busybox* at */*
- mount procfs
- require on half of capabilities: cap_chown, cap_dac_override,
  cap_dac_read_search, cap_fowner, cap_fsetid, cap_kill,
  cap_setgid, cap_setuid, cap_setpcap, cap_linux_immutable,
  cap_net_bind_service, cap_net_broadcast, cap_net_admin,
  cap_net_raw, cap_ipc_lock, cap_ipc_owner, cap_sys_module,
  cap_sys_rawio, cap_sys_chroot, cap_sys_ptrace, cap_sys_pacct,
  cap_sys_admin

Program: *tests/live/data/10-prog*

- print the capability masks using command
  `/busybox grep '^Cap' /proc/self/status`

.REQUIRED-BY @REDPAK.HRQ-R-RED-MAN-LIN-CAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CAP-ENT

.TEST-CASE capabilities2

Check second half of capabilities

Setting: *tests/live/data/11-normal*

- export *busybox* at */*
- mount procfs
- require on half of capabilities: cap_sys_boot,
  cap_sys_nice, cap_sys_resource, cap_sys_time,
  cap_sys_tty_config, cap_mknod, cap_lease, cap_audit_write,
  cap_audit_control, cap_setfcap, cap_mac_override,
  cap_mac_admin, cap_syslog, cap_wake_alarm,
  cap_block_suspend, cap_audit_read, cap_perfmon, cap_bpf,
  cap_checkpoint_restore


Program: *tests/live/data/11-prog*

- print the capability masks using command
  `/busybox grep '^Cap' /proc/self/status`


.REQUIRED-BY @REDPAK.HRQ-R-RED-MAN-LIN-CAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CAP-ENT

## Fuzzing tests

Fuzzing test are planned for 2025. They are not done today for
the following reasons:

- inputs are done by the COTS libyaml + libcyaml and as such are
  supposed to be robust

- final processing is delegated to COTS bubblewrap that is also
  expected to be robust

- feeding redpak with fuzzed data need the development of some
  middleware between the fuzzer and redpak in order to generate
  valid YAML files and avoid almost 100% of test time to be
  just trashed at read step

## Appendice A - Basic test details

### Definition of configurations used for basic tests

- **config-cachedir** : set the item config.cachedir
- **config-capa-cap_chown-0**: drop capability cap_chown
- **config-capa-cap_chown-1**: add capability cap_chown
- **config-capa-cap_setuid-0**: drop capability cap_setuid
- **config-capa-cap_setuid-1**: add capability cap_setuid
- **config-cgrouproot**: set the item config.cgrouproot
- **config-chdir**: set the item config.chdir
- **config-die-with-parent-disabled**: set the item config.die-with-parent to disabled
- **config-die-with-parent-enabled**: set the item config.die-with-parent to enabled
- **config-die-with-parent-unset**: set the item config.die-with-parent to unset
- **config-gpgcheck-false**: set the item config.gpgcheck to false
- **config-gpgcheck-true**: set the item config.gpgcheck to true
- **config-hostname**: set the item config.hostname
- **config-ldpath**: set the item config.ldpath
- **config-new-session-disabled**: set the item config.new-session to disabled
- **config-new-session-enabled**: set the item config.new-session to enabled
- **config-new-session-unset**: set the item config.new-session to unset
- **config-path**: set the item config.path
- **config-persistdir**: set the item config.persistdir
- **config-rpmdir**: set the item config.rpmdir
- **config-share_all-disabled**: set the item config.share_all to disabled
- **config-share_all-enabled**: set the item config.share_all to enabled
- **config-share_all-unset**: set the item config.share_all to unset
- **config-share_cgroup-disabled**: set the item config.share_cgroup to disabled
- **config-share_cgroup-enabled**: set the item config.share_cgroup to enabled
- **config-share_cgroup-unset**: set the item config.share_cgroup to unset
- **config-share_ipc-disabled**: set the item config.share_ipc to disabled
- **config-share_ipc-enabled**: set the item config.share_ipc to enabled
- **config-share_ipc-unset**: set the item config.share_ipc to unset
- **config-share_net-disabled**: set the item config.share_net to disabled
- **config-share_net-enabled**: set the item config.share_net to enabled
- **config-share_net-unset**: set the item config.share_net to unset
- **config-share_pid-disabled**: set the item config.share_pid to disabled
- **config-share_pid-enabled**: set the item config.share_pid to enabled
- **config-share_pid-unset**: set the item config.share_pid to unset
- **config-share_time-disabled**: set the item config.share_time to disabled
- **config-share_time-enabled**: set the item config.share_time to enabled
- **config-share_time-unset**: set the item config.share_time to unset
- **config-share_user-disabled**: set the item config.share_user to disabled
- **config-share_user-enabled**: set the item config.share_user to enabled
- **config-share_user-unset**: set the item config.share_user to unset
- **config-umask-027**: set the item config.umask to 027
- **config-umask-077**: set the item config.umask to 077
- **config-unsafe-false**: set the item config.unsafe to false
- **config-unsafe-true**: set the item config.unsafe to true
- **config-verbose-0**: set the item config.verbose to 0
- **config-verbose-15**: set the item config.verbose to 15
- **environ-default**: declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> '
- **environ-execfd**: declare environment variable VAR as execfd of 'echo HELLO'
- **environ-remove**: declare environment variable VAR as removed
- **environ-static**: declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> '
- **environ-unset**: declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> '
- **export-anonymous-A**: in /A export anonymous
- **export-anonymous-B**: in /B export anonymous
- **export-devfs-A**: in /A export devfs
- **export-devfs-B**: in /B export devfs
- **export-execfd-A**: in /A export execfd of 'echo /somewhere/A'
- **export-execfd-B**: in /B export execfd of 'echo /somewhere/B'
- **export-internal-A**: in /A export as internal /somewhere
- **export-internal-B**: in /B export as internal /somewhere
- **export-lock-A**: in /A export lock
- **export-lock-B**: in /B export lock
- **export-mqueue-A**: in /A export mqueue
- **export-mqueue-B**: in /B export mqueue
- **export-private-A**: in /A export as private /somewhere
- **export-private-B**: in /B export as private /somewhere
- **export-privatefile-A**: in /A export as privatefile /somewhere
- **export-privatefile-B**: in /B export as privatefile /somewhere
- **export-privaterestricted-A**: in /A export as privaterestricted /somewhere
- **export-privaterestricted-B**: in /B export as privaterestricted /somewhere
- **export-procfs-A**: in /A export procfs
- **export-procfs-B**: in /B export procfs
- **export-public-A**: in /A export as public /somewhere
- **export-public-B**: in /B export as public /somewhere
- **export-publicfile-A**: in /A export as publicfile /somewhere
- **export-publicfile-B**: in /B export as publicfile /somewhere
- **export-restricted-A**: in /A export as restricted /somewhere
- **export-restricted-B**: in /B export as restricted /somewhere
- **export-restrictedfile-A**: in /A export as restrictedfile /somewhere
- **export-restrictedfile-B**: in /B export as restrictedfile /somewhere
- **export-symlink-A**: in /A export as symlink /somewhere
- **export-symlink-B**: in /B export as symlink /somewhere
- **export-tmpfs-A**: in /A export tmpfs
- **export-tmpfs-B**: in /B export tmpfs
- **NULL**: nothing, just headers

### Definition of basic tests

#### config-cachedir__NULL

.TEST-CASE config-cachedir__NULL

For this test, CONF1 is config-cachedir (set the item config.cachedir)
and CONF2 is NULL (nothing, just headers).

#### config-capa-cap_chown-0__NULL

.TEST-CASE config-capa-cap_chown-0__NULL

For this test, CONF1 is config-capa-cap_chown-0 (drop capability cap_chown)
and CONF2 is NULL (nothing, just headers).

#### config-capa-cap_chown-1__NULL

.TEST-CASE config-capa-cap_chown-1__NULL

For this test, CONF1 is config-capa-cap_chown-1 (add capability cap_chown)
and CONF2 is NULL (nothing, just headers).

#### config-capa-cap_setuid-0__NULL

.TEST-CASE config-capa-cap_setuid-0__NULL

For this test, CONF1 is config-capa-cap_setuid-0 (drop capability cap_setuid)
and CONF2 is NULL (nothing, just headers).

#### config-capa-cap_setuid-1__NULL

.TEST-CASE config-capa-cap_setuid-1__NULL

For this test, CONF1 is config-capa-cap_setuid-1 (add capability cap_setuid)
and CONF2 is NULL (nothing, just headers).

#### config-cgrouproot__NULL

.TEST-CASE config-cgrouproot__NULL

For this test, CONF1 is config-cgrouproot (set the item config.cgrouproot)
and CONF2 is NULL (nothing, just headers).

#### config-chdir__NULL

.TEST-CASE config-chdir__NULL

For this test, CONF1 is config-chdir (set the item config.chdir)
and CONF2 is NULL (nothing, just headers).

#### config-die-with-parent-disabled__NULL

.TEST-CASE config-die-with-parent-disabled__NULL

For this test, CONF1 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-die-with-parent-enabled__NULL

.TEST-CASE config-die-with-parent-enabled__NULL

For this test, CONF1 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-die-with-parent-unset__NULL

.TEST-CASE config-die-with-parent-unset__NULL

For this test, CONF1 is config-die-with-parent-unset (set the item config.die-with-parent to unset)
and CONF2 is NULL (nothing, just headers).

#### config-gpgcheck-false__NULL

.TEST-CASE config-gpgcheck-false__NULL

For this test, CONF1 is config-gpgcheck-false (set the item config.gpgcheck to false)
and CONF2 is NULL (nothing, just headers).

#### config-gpgcheck-true__NULL

.TEST-CASE config-gpgcheck-true__NULL

For this test, CONF1 is config-gpgcheck-true (set the item config.gpgcheck to true)
and CONF2 is NULL (nothing, just headers).

#### config-hostname__NULL

.TEST-CASE config-hostname__NULL

For this test, CONF1 is config-hostname (set the item config.hostname)
and CONF2 is NULL (nothing, just headers).

#### config-ldpath__NULL

.TEST-CASE config-ldpath__NULL

For this test, CONF1 is config-ldpath (set the item config.ldpath)
and CONF2 is NULL (nothing, just headers).

#### config-new-session-disabled__NULL

.TEST-CASE config-new-session-disabled__NULL

For this test, CONF1 is config-new-session-disabled (set the item config.new-session to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-new-session-enabled__NULL

.TEST-CASE config-new-session-enabled__NULL

For this test, CONF1 is config-new-session-enabled (set the item config.new-session to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-new-session-unset__NULL

.TEST-CASE config-new-session-unset__NULL

For this test, CONF1 is config-new-session-unset (set the item config.new-session to unset)
and CONF2 is NULL (nothing, just headers).

#### config-path__NULL

.TEST-CASE config-path__NULL

For this test, CONF1 is config-path (set the item config.path)
and CONF2 is NULL (nothing, just headers).

#### config-persistdir__NULL

.TEST-CASE config-persistdir__NULL

For this test, CONF1 is config-persistdir (set the item config.persistdir)
and CONF2 is NULL (nothing, just headers).

#### config-rpmdir__NULL

.TEST-CASE config-rpmdir__NULL

For this test, CONF1 is config-rpmdir (set the item config.rpmdir)
and CONF2 is NULL (nothing, just headers).

#### config-share_all-disabled__NULL

.TEST-CASE config-share_all-disabled__NULL

For this test, CONF1 is config-share_all-disabled (set the item config.share_all to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_all-enabled__NULL

.TEST-CASE config-share_all-enabled__NULL

For this test, CONF1 is config-share_all-enabled (set the item config.share_all to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_all-unset__NULL

.TEST-CASE config-share_all-unset__NULL

For this test, CONF1 is config-share_all-unset (set the item config.share_all to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_cgroup-disabled__NULL

.TEST-CASE config-share_cgroup-disabled__NULL

For this test, CONF1 is config-share_cgroup-disabled (set the item config.share_cgroup to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_cgroup-enabled__NULL

.TEST-CASE config-share_cgroup-enabled__NULL

For this test, CONF1 is config-share_cgroup-enabled (set the item config.share_cgroup to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_cgroup-unset__NULL

.TEST-CASE config-share_cgroup-unset__NULL

For this test, CONF1 is config-share_cgroup-unset (set the item config.share_cgroup to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_ipc-disabled__NULL

.TEST-CASE config-share_ipc-disabled__NULL

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_ipc-enabled__NULL

.TEST-CASE config-share_ipc-enabled__NULL

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_ipc-unset__NULL

.TEST-CASE config-share_ipc-unset__NULL

For this test, CONF1 is config-share_ipc-unset (set the item config.share_ipc to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_net-disabled__NULL

.TEST-CASE config-share_net-disabled__NULL

For this test, CONF1 is config-share_net-disabled (set the item config.share_net to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_net-enabled__NULL

.TEST-CASE config-share_net-enabled__NULL

For this test, CONF1 is config-share_net-enabled (set the item config.share_net to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_net-unset__NULL

.TEST-CASE config-share_net-unset__NULL

For this test, CONF1 is config-share_net-unset (set the item config.share_net to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_pid-disabled__NULL

.TEST-CASE config-share_pid-disabled__NULL

For this test, CONF1 is config-share_pid-disabled (set the item config.share_pid to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_pid-enabled__NULL

.TEST-CASE config-share_pid-enabled__NULL

For this test, CONF1 is config-share_pid-enabled (set the item config.share_pid to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_pid-unset__NULL

.TEST-CASE config-share_pid-unset__NULL

For this test, CONF1 is config-share_pid-unset (set the item config.share_pid to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_time-disabled__NULL

.TEST-CASE config-share_time-disabled__NULL

For this test, CONF1 is config-share_time-disabled (set the item config.share_time to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_time-enabled__NULL

.TEST-CASE config-share_time-enabled__NULL

For this test, CONF1 is config-share_time-enabled (set the item config.share_time to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_time-unset__NULL

.TEST-CASE config-share_time-unset__NULL

For this test, CONF1 is config-share_time-unset (set the item config.share_time to unset)
and CONF2 is NULL (nothing, just headers).

#### config-share_user-disabled__NULL

.TEST-CASE config-share_user-disabled__NULL

For this test, CONF1 is config-share_user-disabled (set the item config.share_user to disabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_user-enabled__NULL

.TEST-CASE config-share_user-enabled__NULL

For this test, CONF1 is config-share_user-enabled (set the item config.share_user to enabled)
and CONF2 is NULL (nothing, just headers).

#### config-share_user-unset__NULL

.TEST-CASE config-share_user-unset__NULL

For this test, CONF1 is config-share_user-unset (set the item config.share_user to unset)
and CONF2 is NULL (nothing, just headers).

#### config-umask-027__NULL

.TEST-CASE config-umask-027__NULL

For this test, CONF1 is config-umask-027 (set the item config.umask to 027)
and CONF2 is NULL (nothing, just headers).

#### config-umask-077__NULL

.TEST-CASE config-umask-077__NULL

For this test, CONF1 is config-umask-077 (set the item config.umask to 077)
and CONF2 is NULL (nothing, just headers).

#### config-unsafe-false__NULL

.TEST-CASE config-unsafe-false__NULL

For this test, CONF1 is config-unsafe-false (set the item config.unsafe to false)
and CONF2 is NULL (nothing, just headers).

#### config-unsafe-true__NULL

.TEST-CASE config-unsafe-true__NULL

For this test, CONF1 is config-unsafe-true (set the item config.unsafe to true)
and CONF2 is NULL (nothing, just headers).

#### config-verbose-0__NULL

.TEST-CASE config-verbose-0__NULL

For this test, CONF1 is config-verbose-0 (set the item config.verbose to 0)
and CONF2 is NULL (nothing, just headers).

#### config-verbose-15__NULL

.TEST-CASE config-verbose-15__NULL

For this test, CONF1 is config-verbose-15 (set the item config.verbose to 15)
and CONF2 is NULL (nothing, just headers).

#### environ-default__NULL

.TEST-CASE environ-default__NULL

For this test, CONF1 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is NULL (nothing, just headers).

#### environ-execfd__NULL

.TEST-CASE environ-execfd__NULL

For this test, CONF1 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO')
and CONF2 is NULL (nothing, just headers).

#### environ-remove__NULL

.TEST-CASE environ-remove__NULL

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is NULL (nothing, just headers).

#### environ-static__NULL

.TEST-CASE environ-static__NULL

For this test, CONF1 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is NULL (nothing, just headers).

#### environ-unset__NULL

.TEST-CASE environ-unset__NULL

For this test, CONF1 is environ-unset (declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is NULL (nothing, just headers).

#### export-anonymous-A__NULL

.TEST-CASE export-anonymous-A__NULL

For this test, CONF1 is export-anonymous-A (in /A export anonymous)
and CONF2 is NULL (nothing, just headers).

#### export-devfs-A__NULL

.TEST-CASE export-devfs-A__NULL

For this test, CONF1 is export-devfs-A (in /A export devfs)
and CONF2 is NULL (nothing, just headers).

#### export-execfd-A__NULL

.TEST-CASE export-execfd-A__NULL

For this test, CONF1 is export-execfd-A (in /A export execfd of 'echo /somewhere/A')
and CONF2 is NULL (nothing, just headers).

#### export-internal-A__NULL

.TEST-CASE export-internal-A__NULL

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-lock-A__NULL

.TEST-CASE export-lock-A__NULL

For this test, CONF1 is export-lock-A (in /A export lock)
and CONF2 is NULL (nothing, just headers).

#### export-mqueue-A__NULL

.TEST-CASE export-mqueue-A__NULL

For this test, CONF1 is export-mqueue-A (in /A export mqueue)
and CONF2 is NULL (nothing, just headers).

#### export-private-A__NULL

.TEST-CASE export-private-A__NULL

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-privatefile-A__NULL

.TEST-CASE export-privatefile-A__NULL

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-privaterestricted-A__NULL

.TEST-CASE export-privaterestricted-A__NULL

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-procfs-A__NULL

.TEST-CASE export-procfs-A__NULL

For this test, CONF1 is export-procfs-A (in /A export procfs)
and CONF2 is NULL (nothing, just headers).

#### export-public-A__NULL

.TEST-CASE export-public-A__NULL

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-publicfile-A__NULL

.TEST-CASE export-publicfile-A__NULL

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-restricted-A__NULL

.TEST-CASE export-restricted-A__NULL

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-restrictedfile-A__NULL

.TEST-CASE export-restrictedfile-A__NULL

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-symlink-A__NULL

.TEST-CASE export-symlink-A__NULL

For this test, CONF1 is export-symlink-A (in /A export as symlink /somewhere)
and CONF2 is NULL (nothing, just headers).

#### export-tmpfs-A__NULL

.TEST-CASE export-tmpfs-A__NULL

For this test, CONF1 is export-tmpfs-A (in /A export tmpfs)
and CONF2 is NULL (nothing, just headers).

#### config-cachedir__config-cachedir

.TEST-CASE config-cachedir__config-cachedir

For this test, CONF1 is config-cachedir (set the item config.cachedir)
and CONF2 is config-cachedir (set the item config.cachedir).

#### config-cgrouproot__config-cgrouproot

.TEST-CASE config-cgrouproot__config-cgrouproot

For this test, CONF1 is config-cgrouproot (set the item config.cgrouproot)
and CONF2 is config-cgrouproot (set the item config.cgrouproot).

#### config-chdir__config-chdir

.TEST-CASE config-chdir__config-chdir

For this test, CONF1 is config-chdir (set the item config.chdir)
and CONF2 is config-chdir (set the item config.chdir).

#### config-hostname__config-hostname

.TEST-CASE config-hostname__config-hostname

For this test, CONF1 is config-hostname (set the item config.hostname)
and CONF2 is config-hostname (set the item config.hostname).

#### config-ldpath__config-ldpath

.TEST-CASE config-ldpath__config-ldpath

For this test, CONF1 is config-ldpath (set the item config.ldpath)
and CONF2 is config-ldpath (set the item config.ldpath).

#### config-path__config-path

.TEST-CASE config-path__config-path

For this test, CONF1 is config-path (set the item config.path)
and CONF2 is config-path (set the item config.path).

#### config-persistdir__config-persistdir

.TEST-CASE config-persistdir__config-persistdir

For this test, CONF1 is config-persistdir (set the item config.persistdir)
and CONF2 is config-persistdir (set the item config.persistdir).

#### config-rpmdir__config-rpmdir

.TEST-CASE config-rpmdir__config-rpmdir

For this test, CONF1 is config-rpmdir (set the item config.rpmdir)
and CONF2 is config-rpmdir (set the item config.rpmdir).

#### config-capa-cap_chown-0__config-capa-cap_chown-0

.TEST-CASE config-capa-cap_chown-0__config-capa-cap_chown-0

For this test, CONF1 is config-capa-cap_chown-0 (drop capability cap_chown)
and CONF2 is config-capa-cap_chown-0 (drop capability cap_chown).

#### config-capa-cap_chown-0__config-capa-cap_chown-1

.TEST-CASE config-capa-cap_chown-0__config-capa-cap_chown-1

For this test, CONF1 is config-capa-cap_chown-0 (drop capability cap_chown)
and CONF2 is config-capa-cap_chown-1 (add capability cap_chown).

#### config-capa-cap_chown-0__config-capa-cap_setuid-0

.TEST-CASE config-capa-cap_chown-0__config-capa-cap_setuid-0

For this test, CONF1 is config-capa-cap_chown-0 (drop capability cap_chown)
and CONF2 is config-capa-cap_setuid-0 (drop capability cap_setuid).

#### config-capa-cap_chown-0__config-capa-cap_setuid-1

.TEST-CASE config-capa-cap_chown-0__config-capa-cap_setuid-1

For this test, CONF1 is config-capa-cap_chown-0 (drop capability cap_chown)
and CONF2 is config-capa-cap_setuid-1 (add capability cap_setuid).

#### config-capa-cap_chown-1__config-capa-cap_chown-0

.TEST-CASE config-capa-cap_chown-1__config-capa-cap_chown-0

For this test, CONF1 is config-capa-cap_chown-1 (add capability cap_chown)
and CONF2 is config-capa-cap_chown-0 (drop capability cap_chown).

#### config-capa-cap_chown-1__config-capa-cap_chown-1

.TEST-CASE config-capa-cap_chown-1__config-capa-cap_chown-1

For this test, CONF1 is config-capa-cap_chown-1 (add capability cap_chown)
and CONF2 is config-capa-cap_chown-1 (add capability cap_chown).

#### config-capa-cap_chown-1__config-capa-cap_setuid-0

.TEST-CASE config-capa-cap_chown-1__config-capa-cap_setuid-0

For this test, CONF1 is config-capa-cap_chown-1 (add capability cap_chown)
and CONF2 is config-capa-cap_setuid-0 (drop capability cap_setuid).

#### config-capa-cap_chown-1__config-capa-cap_setuid-1

.TEST-CASE config-capa-cap_chown-1__config-capa-cap_setuid-1

For this test, CONF1 is config-capa-cap_chown-1 (add capability cap_chown)
and CONF2 is config-capa-cap_setuid-1 (add capability cap_setuid).

#### config-capa-cap_setuid-0__config-capa-cap_chown-0

.TEST-CASE config-capa-cap_setuid-0__config-capa-cap_chown-0

For this test, CONF1 is config-capa-cap_setuid-0 (drop capability cap_setuid)
and CONF2 is config-capa-cap_chown-0 (drop capability cap_chown).

#### config-capa-cap_setuid-0__config-capa-cap_chown-1

.TEST-CASE config-capa-cap_setuid-0__config-capa-cap_chown-1

For this test, CONF1 is config-capa-cap_setuid-0 (drop capability cap_setuid)
and CONF2 is config-capa-cap_chown-1 (add capability cap_chown).

#### config-capa-cap_setuid-0__config-capa-cap_setuid-0

.TEST-CASE config-capa-cap_setuid-0__config-capa-cap_setuid-0

For this test, CONF1 is config-capa-cap_setuid-0 (drop capability cap_setuid)
and CONF2 is config-capa-cap_setuid-0 (drop capability cap_setuid).

#### config-capa-cap_setuid-0__config-capa-cap_setuid-1

.TEST-CASE config-capa-cap_setuid-0__config-capa-cap_setuid-1

For this test, CONF1 is config-capa-cap_setuid-0 (drop capability cap_setuid)
and CONF2 is config-capa-cap_setuid-1 (add capability cap_setuid).

#### config-capa-cap_setuid-1__config-capa-cap_chown-0

.TEST-CASE config-capa-cap_setuid-1__config-capa-cap_chown-0

For this test, CONF1 is config-capa-cap_setuid-1 (add capability cap_setuid)
and CONF2 is config-capa-cap_chown-0 (drop capability cap_chown).

#### config-capa-cap_setuid-1__config-capa-cap_chown-1

.TEST-CASE config-capa-cap_setuid-1__config-capa-cap_chown-1

For this test, CONF1 is config-capa-cap_setuid-1 (add capability cap_setuid)
and CONF2 is config-capa-cap_chown-1 (add capability cap_chown).

#### config-capa-cap_setuid-1__config-capa-cap_setuid-0

.TEST-CASE config-capa-cap_setuid-1__config-capa-cap_setuid-0

For this test, CONF1 is config-capa-cap_setuid-1 (add capability cap_setuid)
and CONF2 is config-capa-cap_setuid-0 (drop capability cap_setuid).

#### config-capa-cap_setuid-1__config-capa-cap_setuid-1

.TEST-CASE config-capa-cap_setuid-1__config-capa-cap_setuid-1

For this test, CONF1 is config-capa-cap_setuid-1 (add capability cap_setuid)
and CONF2 is config-capa-cap_setuid-1 (add capability cap_setuid).

#### config-die-with-parent-disabled__config-die-with-parent-disabled

.TEST-CASE config-die-with-parent-disabled__config-die-with-parent-disabled

For this test, CONF1 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled)
and CONF2 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled).

#### config-die-with-parent-disabled__config-die-with-parent-enabled

.TEST-CASE config-die-with-parent-disabled__config-die-with-parent-enabled

For this test, CONF1 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled)
and CONF2 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled).

#### config-die-with-parent-disabled__config-die-with-parent-unset

.TEST-CASE config-die-with-parent-disabled__config-die-with-parent-unset

For this test, CONF1 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled)
and CONF2 is config-die-with-parent-unset (set the item config.die-with-parent to unset).

#### config-die-with-parent-enabled__config-die-with-parent-disabled

.TEST-CASE config-die-with-parent-enabled__config-die-with-parent-disabled

For this test, CONF1 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled)
and CONF2 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled).

#### config-die-with-parent-enabled__config-die-with-parent-enabled

.TEST-CASE config-die-with-parent-enabled__config-die-with-parent-enabled

For this test, CONF1 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled)
and CONF2 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled).

#### config-die-with-parent-enabled__config-die-with-parent-unset

.TEST-CASE config-die-with-parent-enabled__config-die-with-parent-unset

For this test, CONF1 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled)
and CONF2 is config-die-with-parent-unset (set the item config.die-with-parent to unset).

#### config-die-with-parent-unset__config-die-with-parent-disabled

.TEST-CASE config-die-with-parent-unset__config-die-with-parent-disabled

For this test, CONF1 is config-die-with-parent-unset (set the item config.die-with-parent to unset)
and CONF2 is config-die-with-parent-disabled (set the item config.die-with-parent to disabled).

#### config-die-with-parent-unset__config-die-with-parent-enabled

.TEST-CASE config-die-with-parent-unset__config-die-with-parent-enabled

For this test, CONF1 is config-die-with-parent-unset (set the item config.die-with-parent to unset)
and CONF2 is config-die-with-parent-enabled (set the item config.die-with-parent to enabled).

#### config-die-with-parent-unset__config-die-with-parent-unset

.TEST-CASE config-die-with-parent-unset__config-die-with-parent-unset

For this test, CONF1 is config-die-with-parent-unset (set the item config.die-with-parent to unset)
and CONF2 is config-die-with-parent-unset (set the item config.die-with-parent to unset).

#### config-gpgcheck-false__config-gpgcheck-false

.TEST-CASE config-gpgcheck-false__config-gpgcheck-false

For this test, CONF1 is config-gpgcheck-false (set the item config.gpgcheck to false)
and CONF2 is config-gpgcheck-false (set the item config.gpgcheck to false).

#### config-gpgcheck-false__config-gpgcheck-true

.TEST-CASE config-gpgcheck-false__config-gpgcheck-true

For this test, CONF1 is config-gpgcheck-false (set the item config.gpgcheck to false)
and CONF2 is config-gpgcheck-true (set the item config.gpgcheck to true).

#### config-gpgcheck-true__config-gpgcheck-false

.TEST-CASE config-gpgcheck-true__config-gpgcheck-false

For this test, CONF1 is config-gpgcheck-true (set the item config.gpgcheck to true)
and CONF2 is config-gpgcheck-false (set the item config.gpgcheck to false).

#### config-gpgcheck-true__config-gpgcheck-true

.TEST-CASE config-gpgcheck-true__config-gpgcheck-true

For this test, CONF1 is config-gpgcheck-true (set the item config.gpgcheck to true)
and CONF2 is config-gpgcheck-true (set the item config.gpgcheck to true).

#### config-new-session-disabled__config-new-session-disabled

.TEST-CASE config-new-session-disabled__config-new-session-disabled

For this test, CONF1 is config-new-session-disabled (set the item config.new-session to disabled)
and CONF2 is config-new-session-disabled (set the item config.new-session to disabled).

#### config-new-session-disabled__config-new-session-enabled

.TEST-CASE config-new-session-disabled__config-new-session-enabled

For this test, CONF1 is config-new-session-disabled (set the item config.new-session to disabled)
and CONF2 is config-new-session-enabled (set the item config.new-session to enabled).

#### config-new-session-enabled__config-new-session-disabled

.TEST-CASE config-new-session-enabled__config-new-session-disabled

For this test, CONF1 is config-new-session-enabled (set the item config.new-session to enabled)
and CONF2 is config-new-session-disabled (set the item config.new-session to disabled).

#### config-new-session-enabled__config-new-session-enabled

.TEST-CASE config-new-session-enabled__config-new-session-enabled

For this test, CONF1 is config-new-session-enabled (set the item config.new-session to enabled)
and CONF2 is config-new-session-enabled (set the item config.new-session to enabled).

#### config-share_all-disabled__config-share_all-disabled

.TEST-CASE config-share_all-disabled__config-share_all-disabled

For this test, CONF1 is config-share_all-disabled (set the item config.share_all to disabled)
and CONF2 is config-share_all-disabled (set the item config.share_all to disabled).

#### config-share_all-disabled__config-share_all-enabled

.TEST-CASE config-share_all-disabled__config-share_all-enabled

For this test, CONF1 is config-share_all-disabled (set the item config.share_all to disabled)
and CONF2 is config-share_all-enabled (set the item config.share_all to enabled).

#### config-share_all-disabled__config-share_ipc-disabled

.TEST-CASE config-share_all-disabled__config-share_ipc-disabled

For this test, CONF1 is config-share_all-disabled (set the item config.share_all to disabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_all-disabled__config-share_ipc-enabled

.TEST-CASE config-share_all-disabled__config-share_ipc-enabled

For this test, CONF1 is config-share_all-disabled (set the item config.share_all to disabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_all-enabled__config-share_all-disabled

.TEST-CASE config-share_all-enabled__config-share_all-disabled

For this test, CONF1 is config-share_all-enabled (set the item config.share_all to enabled)
and CONF2 is config-share_all-disabled (set the item config.share_all to disabled).

#### config-share_all-enabled__config-share_all-enabled

.TEST-CASE config-share_all-enabled__config-share_all-enabled

For this test, CONF1 is config-share_all-enabled (set the item config.share_all to enabled)
and CONF2 is config-share_all-enabled (set the item config.share_all to enabled).

#### config-share_all-enabled__config-share_ipc-disabled

.TEST-CASE config-share_all-enabled__config-share_ipc-disabled

For this test, CONF1 is config-share_all-enabled (set the item config.share_all to enabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_all-enabled__config-share_ipc-enabled

.TEST-CASE config-share_all-enabled__config-share_ipc-enabled

For this test, CONF1 is config-share_all-enabled (set the item config.share_all to enabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_ipc-disabled__config-share_all-disabled

.TEST-CASE config-share_ipc-disabled__config-share_all-disabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_all-disabled (set the item config.share_all to disabled).

#### config-share_ipc-disabled__config-share_all-enabled

.TEST-CASE config-share_ipc-disabled__config-share_all-enabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_all-enabled (set the item config.share_all to enabled).

#### config-share_ipc-disabled__config-share_ipc-disabled

.TEST-CASE config-share_ipc-disabled__config-share_ipc-disabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_ipc-disabled__config-share_ipc-enabled

.TEST-CASE config-share_ipc-disabled__config-share_ipc-enabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_ipc-enabled__config-share_all-disabled

.TEST-CASE config-share_ipc-enabled__config-share_all-disabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_all-disabled (set the item config.share_all to disabled).

#### config-share_ipc-enabled__config-share_all-enabled

.TEST-CASE config-share_ipc-enabled__config-share_all-enabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_all-enabled (set the item config.share_all to enabled).

#### config-share_ipc-enabled__config-share_ipc-disabled

.TEST-CASE config-share_ipc-enabled__config-share_ipc-disabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_ipc-enabled__config-share_ipc-enabled

.TEST-CASE config-share_ipc-enabled__config-share_ipc-enabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_ipc-disabled__config-share_ipc-disabled

.TEST-CASE config-share_ipc-disabled__config-share_ipc-disabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_ipc-disabled__config-share_ipc-enabled

.TEST-CASE config-share_ipc-disabled__config-share_ipc-enabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_ipc-disabled__config-share_net-disabled

.TEST-CASE config-share_ipc-disabled__config-share_net-disabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_net-disabled (set the item config.share_net to disabled).

#### config-share_ipc-disabled__config-share_net-enabled

.TEST-CASE config-share_ipc-disabled__config-share_net-enabled

For this test, CONF1 is config-share_ipc-disabled (set the item config.share_ipc to disabled)
and CONF2 is config-share_net-enabled (set the item config.share_net to enabled).

#### config-share_ipc-enabled__config-share_ipc-disabled

.TEST-CASE config-share_ipc-enabled__config-share_ipc-disabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_ipc-enabled__config-share_ipc-enabled

.TEST-CASE config-share_ipc-enabled__config-share_ipc-enabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_ipc-enabled__config-share_net-disabled

.TEST-CASE config-share_ipc-enabled__config-share_net-disabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_net-disabled (set the item config.share_net to disabled).

#### config-share_ipc-enabled__config-share_net-enabled

.TEST-CASE config-share_ipc-enabled__config-share_net-enabled

For this test, CONF1 is config-share_ipc-enabled (set the item config.share_ipc to enabled)
and CONF2 is config-share_net-enabled (set the item config.share_net to enabled).

#### config-share_net-disabled__config-share_ipc-disabled

.TEST-CASE config-share_net-disabled__config-share_ipc-disabled

For this test, CONF1 is config-share_net-disabled (set the item config.share_net to disabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_net-disabled__config-share_ipc-enabled

.TEST-CASE config-share_net-disabled__config-share_ipc-enabled

For this test, CONF1 is config-share_net-disabled (set the item config.share_net to disabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_net-disabled__config-share_net-disabled

.TEST-CASE config-share_net-disabled__config-share_net-disabled

For this test, CONF1 is config-share_net-disabled (set the item config.share_net to disabled)
and CONF2 is config-share_net-disabled (set the item config.share_net to disabled).

#### config-share_net-disabled__config-share_net-enabled

.TEST-CASE config-share_net-disabled__config-share_net-enabled

For this test, CONF1 is config-share_net-disabled (set the item config.share_net to disabled)
and CONF2 is config-share_net-enabled (set the item config.share_net to enabled).

#### config-share_net-enabled__config-share_ipc-disabled

.TEST-CASE config-share_net-enabled__config-share_ipc-disabled

For this test, CONF1 is config-share_net-enabled (set the item config.share_net to enabled)
and CONF2 is config-share_ipc-disabled (set the item config.share_ipc to disabled).

#### config-share_net-enabled__config-share_ipc-enabled

.TEST-CASE config-share_net-enabled__config-share_ipc-enabled

For this test, CONF1 is config-share_net-enabled (set the item config.share_net to enabled)
and CONF2 is config-share_ipc-enabled (set the item config.share_ipc to enabled).

#### config-share_net-enabled__config-share_net-disabled

.TEST-CASE config-share_net-enabled__config-share_net-disabled

For this test, CONF1 is config-share_net-enabled (set the item config.share_net to enabled)
and CONF2 is config-share_net-disabled (set the item config.share_net to disabled).

#### config-share_net-enabled__config-share_net-enabled

.TEST-CASE config-share_net-enabled__config-share_net-enabled

For this test, CONF1 is config-share_net-enabled (set the item config.share_net to enabled)
and CONF2 is config-share_net-enabled (set the item config.share_net to enabled).

#### config-umask-027__config-umask-027

.TEST-CASE config-umask-027__config-umask-027

For this test, CONF1 is config-umask-027 (set the item config.umask to 027)
and CONF2 is config-umask-027 (set the item config.umask to 027).

#### config-umask-027__config-umask-077

.TEST-CASE config-umask-027__config-umask-077

For this test, CONF1 is config-umask-027 (set the item config.umask to 027)
and CONF2 is config-umask-077 (set the item config.umask to 077).

#### config-umask-077__config-umask-027

.TEST-CASE config-umask-077__config-umask-027

For this test, CONF1 is config-umask-077 (set the item config.umask to 077)
and CONF2 is config-umask-027 (set the item config.umask to 027).

#### config-umask-077__config-umask-077

.TEST-CASE config-umask-077__config-umask-077

For this test, CONF1 is config-umask-077 (set the item config.umask to 077)
and CONF2 is config-umask-077 (set the item config.umask to 077).

#### environ-default__environ-default

.TEST-CASE environ-default__environ-default

For this test, CONF1 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-default__environ-execfd

.TEST-CASE environ-default__environ-execfd

For this test, CONF1 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO').

#### environ-default__environ-remove

.TEST-CASE environ-default__environ-remove

For this test, CONF1 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-default__environ-static

.TEST-CASE environ-default__environ-static

For this test, CONF1 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-execfd__environ-default

.TEST-CASE environ-execfd__environ-default

For this test, CONF1 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO')
and CONF2 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-execfd__environ-execfd

.TEST-CASE environ-execfd__environ-execfd

For this test, CONF1 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO')
and CONF2 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO').

#### environ-execfd__environ-remove

.TEST-CASE environ-execfd__environ-remove

For this test, CONF1 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO')
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-execfd__environ-static

.TEST-CASE environ-execfd__environ-static

For this test, CONF1 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO')
and CONF2 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-remove__environ-default

.TEST-CASE environ-remove__environ-default

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-remove__environ-execfd

.TEST-CASE environ-remove__environ-execfd

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO').

#### environ-remove__environ-remove

.TEST-CASE environ-remove__environ-remove

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-remove__environ-static

.TEST-CASE environ-remove__environ-static

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-static__environ-default

.TEST-CASE environ-static__environ-default

For this test, CONF1 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-default (declare environment variable VAR as default of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-static__environ-execfd

.TEST-CASE environ-static__environ-execfd

For this test, CONF1 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-execfd (declare environment variable VAR as execfd of 'echo HELLO').

#### environ-static__environ-remove

.TEST-CASE environ-static__environ-remove

For this test, CONF1 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-static__environ-static

.TEST-CASE environ-static__environ-static

For this test, CONF1 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-static (declare environment variable VAR as static of '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-remove__environ-remove

.TEST-CASE environ-remove__environ-remove

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-remove__environ-unset

.TEST-CASE environ-remove__environ-unset

For this test, CONF1 is environ-remove (declare environment variable VAR as removed)
and CONF2 is environ-unset (declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### environ-unset__environ-remove

.TEST-CASE environ-unset__environ-remove

For this test, CONF1 is environ-unset (declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-remove (declare environment variable VAR as removed).

#### environ-unset__environ-unset

.TEST-CASE environ-unset__environ-unset

For this test, CONF1 is environ-unset (declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> ')
and CONF2 is environ-unset (declare environment variable VAR as '\$NODE_ALIAS(\$LEAF\_ALIAS)> ').

#### export-anonymous-A__export-anonymous-A

.TEST-CASE export-anonymous-A__export-anonymous-A

For this test, CONF1 is export-anonymous-A (in /A export anonymous)
and CONF2 is export-anonymous-A (in /A export anonymous).

#### export-anonymous-A__export-anonymous-B

.TEST-CASE export-anonymous-A__export-anonymous-B

For this test, CONF1 is export-anonymous-A (in /A export anonymous)
and CONF2 is export-anonymous-B (in /B export anonymous).

#### export-anonymous-B__export-anonymous-A

.TEST-CASE export-anonymous-B__export-anonymous-A

For this test, CONF1 is export-anonymous-B (in /B export anonymous)
and CONF2 is export-anonymous-A (in /A export anonymous).

#### export-anonymous-B__export-anonymous-B

.TEST-CASE export-anonymous-B__export-anonymous-B

For this test, CONF1 is export-anonymous-B (in /B export anonymous)
and CONF2 is export-anonymous-B (in /B export anonymous).

#### export-devfs-A__export-devfs-A

.TEST-CASE export-devfs-A__export-devfs-A

For this test, CONF1 is export-devfs-A (in /A export devfs)
and CONF2 is export-devfs-A (in /A export devfs).

#### export-devfs-A__export-devfs-B

.TEST-CASE export-devfs-A__export-devfs-B

For this test, CONF1 is export-devfs-A (in /A export devfs)
and CONF2 is export-devfs-B (in /B export devfs).

#### export-devfs-B__export-devfs-A

.TEST-CASE export-devfs-B__export-devfs-A

For this test, CONF1 is export-devfs-B (in /B export devfs)
and CONF2 is export-devfs-A (in /A export devfs).

#### export-devfs-B__export-devfs-B

.TEST-CASE export-devfs-B__export-devfs-B

For this test, CONF1 is export-devfs-B (in /B export devfs)
and CONF2 is export-devfs-B (in /B export devfs).

#### export-execfd-A__export-execfd-A

.TEST-CASE export-execfd-A__export-execfd-A

For this test, CONF1 is export-execfd-A (in /A export execfd of 'echo /somewhere/A')
and CONF2 is export-execfd-A (in /A export execfd of 'echo /somewhere/A').

#### export-execfd-A__export-execfd-B

.TEST-CASE export-execfd-A__export-execfd-B

For this test, CONF1 is export-execfd-A (in /A export execfd of 'echo /somewhere/A')
and CONF2 is export-execfd-B (in /B export execfd of 'echo /somewhere/B').

#### export-execfd-B__export-execfd-A

.TEST-CASE export-execfd-B__export-execfd-A

For this test, CONF1 is export-execfd-B (in /B export execfd of 'echo /somewhere/B')
and CONF2 is export-execfd-A (in /A export execfd of 'echo /somewhere/A').

#### export-execfd-B__export-execfd-B

.TEST-CASE export-execfd-B__export-execfd-B

For this test, CONF1 is export-execfd-B (in /B export execfd of 'echo /somewhere/B')
and CONF2 is export-execfd-B (in /B export execfd of 'echo /somewhere/B').

#### export-internal-A__export-internal-A

.TEST-CASE export-internal-A__export-internal-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-internal-A__export-internal-B

.TEST-CASE export-internal-A__export-internal-B

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-internal-B (in /B export as internal /somewhere).

#### export-internal-B__export-internal-A

.TEST-CASE export-internal-B__export-internal-A

For this test, CONF1 is export-internal-B (in /B export as internal /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-internal-B__export-internal-B

.TEST-CASE export-internal-B__export-internal-B

For this test, CONF1 is export-internal-B (in /B export as internal /somewhere)
and CONF2 is export-internal-B (in /B export as internal /somewhere).

#### export-private-A__export-private-A

.TEST-CASE export-private-A__export-private-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-private-A__export-private-B

.TEST-CASE export-private-A__export-private-B

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-private-B (in /B export as private /somewhere).

#### export-private-B__export-private-A

.TEST-CASE export-private-B__export-private-A

For this test, CONF1 is export-private-B (in /B export as private /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-private-B__export-private-B

.TEST-CASE export-private-B__export-private-B

For this test, CONF1 is export-private-B (in /B export as private /somewhere)
and CONF2 is export-private-B (in /B export as private /somewhere).

#### export-privatefile-A__export-privatefile-A

.TEST-CASE export-privatefile-A__export-privatefile-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-privatefile-A__export-privatefile-B

.TEST-CASE export-privatefile-A__export-privatefile-B

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-privatefile-B (in /B export as privatefile /somewhere).

#### export-privatefile-B__export-privatefile-A

.TEST-CASE export-privatefile-B__export-privatefile-A

For this test, CONF1 is export-privatefile-B (in /B export as privatefile /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-privatefile-B__export-privatefile-B

.TEST-CASE export-privatefile-B__export-privatefile-B

For this test, CONF1 is export-privatefile-B (in /B export as privatefile /somewhere)
and CONF2 is export-privatefile-B (in /B export as privatefile /somewhere).

#### export-privaterestricted-A__export-privaterestricted-A

.TEST-CASE export-privaterestricted-A__export-privaterestricted-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-privaterestricted-A__export-privaterestricted-B

.TEST-CASE export-privaterestricted-A__export-privaterestricted-B

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-privaterestricted-B (in /B export as privaterestricted /somewhere).

#### export-privaterestricted-B__export-privaterestricted-A

.TEST-CASE export-privaterestricted-B__export-privaterestricted-A

For this test, CONF1 is export-privaterestricted-B (in /B export as privaterestricted /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-privaterestricted-B__export-privaterestricted-B

.TEST-CASE export-privaterestricted-B__export-privaterestricted-B

For this test, CONF1 is export-privaterestricted-B (in /B export as privaterestricted /somewhere)
and CONF2 is export-privaterestricted-B (in /B export as privaterestricted /somewhere).

#### export-public-A__export-public-A

.TEST-CASE export-public-A__export-public-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-public-A__export-public-B

.TEST-CASE export-public-A__export-public-B

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-public-B (in /B export as public /somewhere).

#### export-public-B__export-public-A

.TEST-CASE export-public-B__export-public-A

For this test, CONF1 is export-public-B (in /B export as public /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-public-B__export-public-B

.TEST-CASE export-public-B__export-public-B

For this test, CONF1 is export-public-B (in /B export as public /somewhere)
and CONF2 is export-public-B (in /B export as public /somewhere).

#### export-publicfile-A__export-publicfile-A

.TEST-CASE export-publicfile-A__export-publicfile-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-publicfile-A__export-publicfile-B

.TEST-CASE export-publicfile-A__export-publicfile-B

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-publicfile-B (in /B export as publicfile /somewhere).

#### export-publicfile-B__export-publicfile-A

.TEST-CASE export-publicfile-B__export-publicfile-A

For this test, CONF1 is export-publicfile-B (in /B export as publicfile /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-publicfile-B__export-publicfile-B

.TEST-CASE export-publicfile-B__export-publicfile-B

For this test, CONF1 is export-publicfile-B (in /B export as publicfile /somewhere)
and CONF2 is export-publicfile-B (in /B export as publicfile /somewhere).

#### export-restricted-A__export-restricted-A

.TEST-CASE export-restricted-A__export-restricted-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-restricted-A__export-restricted-B

.TEST-CASE export-restricted-A__export-restricted-B

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-restricted-B (in /B export as restricted /somewhere).

#### export-restricted-B__export-restricted-A

.TEST-CASE export-restricted-B__export-restricted-A

For this test, CONF1 is export-restricted-B (in /B export as restricted /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-restricted-B__export-restricted-B

.TEST-CASE export-restricted-B__export-restricted-B

For this test, CONF1 is export-restricted-B (in /B export as restricted /somewhere)
and CONF2 is export-restricted-B (in /B export as restricted /somewhere).

#### export-restrictedfile-A__export-restrictedfile-A

.TEST-CASE export-restrictedfile-A__export-restrictedfile-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-restrictedfile-A__export-restrictedfile-B

.TEST-CASE export-restrictedfile-A__export-restrictedfile-B

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-restrictedfile-B (in /B export as restrictedfile /somewhere).

#### export-restrictedfile-B__export-restrictedfile-A

.TEST-CASE export-restrictedfile-B__export-restrictedfile-A

For this test, CONF1 is export-restrictedfile-B (in /B export as restrictedfile /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-restrictedfile-B__export-restrictedfile-B

.TEST-CASE export-restrictedfile-B__export-restrictedfile-B

For this test, CONF1 is export-restrictedfile-B (in /B export as restrictedfile /somewhere)
and CONF2 is export-restrictedfile-B (in /B export as restrictedfile /somewhere).

#### export-lock-A__export-lock-A

.TEST-CASE export-lock-A__export-lock-A

For this test, CONF1 is export-lock-A (in /A export lock)
and CONF2 is export-lock-A (in /A export lock).

#### export-lock-A__export-lock-B

.TEST-CASE export-lock-A__export-lock-B

For this test, CONF1 is export-lock-A (in /A export lock)
and CONF2 is export-lock-B (in /B export lock).

#### export-lock-B__export-lock-A

.TEST-CASE export-lock-B__export-lock-A

For this test, CONF1 is export-lock-B (in /B export lock)
and CONF2 is export-lock-A (in /A export lock).

#### export-lock-B__export-lock-B

.TEST-CASE export-lock-B__export-lock-B

For this test, CONF1 is export-lock-B (in /B export lock)
and CONF2 is export-lock-B (in /B export lock).

#### export-mqueue-A__export-mqueue-A

.TEST-CASE export-mqueue-A__export-mqueue-A

For this test, CONF1 is export-mqueue-A (in /A export mqueue)
and CONF2 is export-mqueue-A (in /A export mqueue).

#### export-mqueue-A__export-mqueue-B

.TEST-CASE export-mqueue-A__export-mqueue-B

For this test, CONF1 is export-mqueue-A (in /A export mqueue)
and CONF2 is export-mqueue-B (in /B export mqueue).

#### export-mqueue-B__export-mqueue-A

.TEST-CASE export-mqueue-B__export-mqueue-A

For this test, CONF1 is export-mqueue-B (in /B export mqueue)
and CONF2 is export-mqueue-A (in /A export mqueue).

#### export-mqueue-B__export-mqueue-B

.TEST-CASE export-mqueue-B__export-mqueue-B

For this test, CONF1 is export-mqueue-B (in /B export mqueue)
and CONF2 is export-mqueue-B (in /B export mqueue).

#### export-procfs-A__export-procfs-A

.TEST-CASE export-procfs-A__export-procfs-A

For this test, CONF1 is export-procfs-A (in /A export procfs)
and CONF2 is export-procfs-A (in /A export procfs).

#### export-procfs-A__export-procfs-B

.TEST-CASE export-procfs-A__export-procfs-B

For this test, CONF1 is export-procfs-A (in /A export procfs)
and CONF2 is export-procfs-B (in /B export procfs).

#### export-procfs-B__export-procfs-A

.TEST-CASE export-procfs-B__export-procfs-A

For this test, CONF1 is export-procfs-B (in /B export procfs)
and CONF2 is export-procfs-A (in /A export procfs).

#### export-procfs-B__export-procfs-B

.TEST-CASE export-procfs-B__export-procfs-B

For this test, CONF1 is export-procfs-B (in /B export procfs)
and CONF2 is export-procfs-B (in /B export procfs).

#### export-symlink-A__export-symlink-A

.TEST-CASE export-symlink-A__export-symlink-A

For this test, CONF1 is export-symlink-A (in /A export as symlink /somewhere)
and CONF2 is export-symlink-A (in /A export as symlink /somewhere).

#### export-symlink-A__export-symlink-B

.TEST-CASE export-symlink-A__export-symlink-B

For this test, CONF1 is export-symlink-A (in /A export as symlink /somewhere)
and CONF2 is export-symlink-B (in /B export as symlink /somewhere).

#### export-symlink-B__export-symlink-A

.TEST-CASE export-symlink-B__export-symlink-A

For this test, CONF1 is export-symlink-B (in /B export as symlink /somewhere)
and CONF2 is export-symlink-A (in /A export as symlink /somewhere).

#### export-symlink-B__export-symlink-B

.TEST-CASE export-symlink-B__export-symlink-B

For this test, CONF1 is export-symlink-B (in /B export as symlink /somewhere)
and CONF2 is export-symlink-B (in /B export as symlink /somewhere).

#### export-tmpfs-A__export-tmpfs-A

.TEST-CASE export-tmpfs-A__export-tmpfs-A

For this test, CONF1 is export-tmpfs-A (in /A export tmpfs)
and CONF2 is export-tmpfs-A (in /A export tmpfs).

#### export-tmpfs-A__export-tmpfs-B

.TEST-CASE export-tmpfs-A__export-tmpfs-B

For this test, CONF1 is export-tmpfs-A (in /A export tmpfs)
and CONF2 is export-tmpfs-B (in /B export tmpfs).

#### export-tmpfs-B__export-tmpfs-A

.TEST-CASE export-tmpfs-B__export-tmpfs-A

For this test, CONF1 is export-tmpfs-B (in /B export tmpfs)
and CONF2 is export-tmpfs-A (in /A export tmpfs).

#### export-tmpfs-B__export-tmpfs-B

.TEST-CASE export-tmpfs-B__export-tmpfs-B

For this test, CONF1 is export-tmpfs-B (in /B export tmpfs)
and CONF2 is export-tmpfs-B (in /B export tmpfs).

#### export-internal-A__export-internal-A

.TEST-CASE export-internal-A__export-internal-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-internal-A__export-private-A

.TEST-CASE export-internal-A__export-private-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-internal-A__export-privatefile-A

.TEST-CASE export-internal-A__export-privatefile-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-internal-A__export-privaterestricted-A

.TEST-CASE export-internal-A__export-privaterestricted-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-internal-A__export-public-A

.TEST-CASE export-internal-A__export-public-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-internal-A__export-publicfile-A

.TEST-CASE export-internal-A__export-publicfile-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-internal-A__export-restricted-A

.TEST-CASE export-internal-A__export-restricted-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-internal-A__export-restrictedfile-A

.TEST-CASE export-internal-A__export-restrictedfile-A

For this test, CONF1 is export-internal-A (in /A export as internal /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-private-A__export-internal-A

.TEST-CASE export-private-A__export-internal-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-private-A__export-private-A

.TEST-CASE export-private-A__export-private-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-private-A__export-privatefile-A

.TEST-CASE export-private-A__export-privatefile-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-private-A__export-privaterestricted-A

.TEST-CASE export-private-A__export-privaterestricted-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-private-A__export-public-A

.TEST-CASE export-private-A__export-public-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-private-A__export-publicfile-A

.TEST-CASE export-private-A__export-publicfile-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-private-A__export-restricted-A

.TEST-CASE export-private-A__export-restricted-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-private-A__export-restrictedfile-A

.TEST-CASE export-private-A__export-restrictedfile-A

For this test, CONF1 is export-private-A (in /A export as private /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-privatefile-A__export-internal-A

.TEST-CASE export-privatefile-A__export-internal-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-privatefile-A__export-private-A

.TEST-CASE export-privatefile-A__export-private-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-privatefile-A__export-privatefile-A

.TEST-CASE export-privatefile-A__export-privatefile-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-privatefile-A__export-privaterestricted-A

.TEST-CASE export-privatefile-A__export-privaterestricted-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-privatefile-A__export-public-A

.TEST-CASE export-privatefile-A__export-public-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-privatefile-A__export-publicfile-A

.TEST-CASE export-privatefile-A__export-publicfile-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-privatefile-A__export-restricted-A

.TEST-CASE export-privatefile-A__export-restricted-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-privatefile-A__export-restrictedfile-A

.TEST-CASE export-privatefile-A__export-restrictedfile-A

For this test, CONF1 is export-privatefile-A (in /A export as privatefile /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-privaterestricted-A__export-internal-A

.TEST-CASE export-privaterestricted-A__export-internal-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-privaterestricted-A__export-private-A

.TEST-CASE export-privaterestricted-A__export-private-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-privaterestricted-A__export-privatefile-A

.TEST-CASE export-privaterestricted-A__export-privatefile-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-privaterestricted-A__export-privaterestricted-A

.TEST-CASE export-privaterestricted-A__export-privaterestricted-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-privaterestricted-A__export-public-A

.TEST-CASE export-privaterestricted-A__export-public-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-privaterestricted-A__export-publicfile-A

.TEST-CASE export-privaterestricted-A__export-publicfile-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-privaterestricted-A__export-restricted-A

.TEST-CASE export-privaterestricted-A__export-restricted-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-privaterestricted-A__export-restrictedfile-A

.TEST-CASE export-privaterestricted-A__export-restrictedfile-A

For this test, CONF1 is export-privaterestricted-A (in /A export as privaterestricted /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-public-A__export-internal-A

.TEST-CASE export-public-A__export-internal-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-public-A__export-private-A

.TEST-CASE export-public-A__export-private-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-public-A__export-privatefile-A

.TEST-CASE export-public-A__export-privatefile-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-public-A__export-privaterestricted-A

.TEST-CASE export-public-A__export-privaterestricted-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-public-A__export-public-A

.TEST-CASE export-public-A__export-public-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-public-A__export-publicfile-A

.TEST-CASE export-public-A__export-publicfile-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-public-A__export-restricted-A

.TEST-CASE export-public-A__export-restricted-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-public-A__export-restrictedfile-A

.TEST-CASE export-public-A__export-restrictedfile-A

For this test, CONF1 is export-public-A (in /A export as public /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-publicfile-A__export-internal-A

.TEST-CASE export-publicfile-A__export-internal-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-publicfile-A__export-private-A

.TEST-CASE export-publicfile-A__export-private-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-publicfile-A__export-privatefile-A

.TEST-CASE export-publicfile-A__export-privatefile-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-publicfile-A__export-privaterestricted-A

.TEST-CASE export-publicfile-A__export-privaterestricted-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-publicfile-A__export-public-A

.TEST-CASE export-publicfile-A__export-public-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-publicfile-A__export-publicfile-A

.TEST-CASE export-publicfile-A__export-publicfile-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-publicfile-A__export-restricted-A

.TEST-CASE export-publicfile-A__export-restricted-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-publicfile-A__export-restrictedfile-A

.TEST-CASE export-publicfile-A__export-restrictedfile-A

For this test, CONF1 is export-publicfile-A (in /A export as publicfile /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-restricted-A__export-internal-A

.TEST-CASE export-restricted-A__export-internal-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-restricted-A__export-private-A

.TEST-CASE export-restricted-A__export-private-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-restricted-A__export-privatefile-A

.TEST-CASE export-restricted-A__export-privatefile-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-restricted-A__export-privaterestricted-A

.TEST-CASE export-restricted-A__export-privaterestricted-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-restricted-A__export-public-A

.TEST-CASE export-restricted-A__export-public-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-restricted-A__export-publicfile-A

.TEST-CASE export-restricted-A__export-publicfile-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-restricted-A__export-restricted-A

.TEST-CASE export-restricted-A__export-restricted-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-restricted-A__export-restrictedfile-A

.TEST-CASE export-restricted-A__export-restrictedfile-A

For this test, CONF1 is export-restricted-A (in /A export as restricted /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).

#### export-restrictedfile-A__export-internal-A

.TEST-CASE export-restrictedfile-A__export-internal-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-internal-A (in /A export as internal /somewhere).

#### export-restrictedfile-A__export-private-A

.TEST-CASE export-restrictedfile-A__export-private-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-private-A (in /A export as private /somewhere).

#### export-restrictedfile-A__export-privatefile-A

.TEST-CASE export-restrictedfile-A__export-privatefile-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-privatefile-A (in /A export as privatefile /somewhere).

#### export-restrictedfile-A__export-privaterestricted-A

.TEST-CASE export-restrictedfile-A__export-privaterestricted-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-privaterestricted-A (in /A export as privaterestricted /somewhere).

#### export-restrictedfile-A__export-public-A

.TEST-CASE export-restrictedfile-A__export-public-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-public-A (in /A export as public /somewhere).

#### export-restrictedfile-A__export-publicfile-A

.TEST-CASE export-restrictedfile-A__export-publicfile-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-publicfile-A (in /A export as publicfile /somewhere).

#### export-restrictedfile-A__export-restricted-A

.TEST-CASE export-restrictedfile-A__export-restricted-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-restricted-A (in /A export as restricted /somewhere).

#### export-restrictedfile-A__export-restrictedfile-A

.TEST-CASE export-restrictedfile-A__export-restrictedfile-A

For this test, CONF1 is export-restrictedfile-A (in /A export as restrictedfile /somewhere)
and CONF2 is export-restrictedfile-A (in /A export as restrictedfile /somewhere).



## Appendix B - basic test configurations

The script `tests/basic/gen-confs.sh` is dedicated to create
configuration files for basic test.

The configuration files created are listed below.
Many of them is used by basic test however not all.

Each of the below configuration sets only one single entry,
except NULL that sets nothing.

#### NULL

```YAML
headers:
  alias: alias
  name: name
  info: info
```

#### config-share_all-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_all: disabled
```

#### config-share_all-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_all: enabled
```

#### config-share_all-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_all: unset
```

#### config-share_user-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_user: disabled
```

#### config-share_user-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_user: enabled
```

#### config-share_user-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_user: unset
```

#### config-share_cgroup-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_cgroup: disabled
```

#### config-share_cgroup-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_cgroup: enabled
```

#### config-share_cgroup-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_cgroup: unset
```

#### config-share_net-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_net: disabled
```

#### config-share_net-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_net: enabled
```

#### config-share_net-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_net: unset
```

#### config-share_pid-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_pid: disabled
```

#### config-share_pid-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_pid: enabled
```

#### config-share_pid-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_pid: unset
```

#### config-share_ipc-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_ipc: disabled
```

#### config-share_ipc-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_ipc: enabled
```

#### config-share_ipc-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_ipc: unset
```

#### config-share_time-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_time: disabled
```

#### config-share_time-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_time: enabled
```

#### config-share_time-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   share_time: unset
```

#### config-die-with-parent-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   die-with-parent: disabled
```

#### config-die-with-parent-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   die-with-parent: enabled
```

#### config-die-with-parent-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   die-with-parent: unset
```

#### config-new-session-disabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   new-session: disabled
```

#### config-new-session-enabled

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   new-session: enabled
```

#### config-new-session-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   new-session: unset
```

#### config-rpmdir

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   rpmdir: $NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS
```

#### config-persistdir

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   persistdir: $NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS
```

#### config-cachedir

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   cachedir: $NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS
```

#### config-path

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   path: $NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS
```

#### config-ldpath

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   ldpath: $NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS
```

#### config-verbose-0

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   verbose: 0
```

#### config-verbose-15

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   verbose: 15
```

#### config-umask-027

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   verbose: 027
```

#### config-umask-077

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   verbose: 077
```

#### config-hostname

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   hostname: $LEAF_ALIAS
```

#### config-chdir

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   chdir: /home/$LEAF_ALIAS
```

#### config-cgrouproot

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   cgrouproot: /sys/fs/cgroup/user.slice/user-$UID.slice/user@$UID.service
```

#### config-gpgcheck-true

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   gpgcheck: true
```

#### config-gpgcheck-false

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   gpgcheck: false
```

#### config-unsafe-true

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   unsafe: true
```

#### config-unsafe-false

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   unsafe: false
```

#### config-capa-cap_chown-0

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   capabilities:
     - cap: cap_chown
       add: 0
```

#### config-capa-cap_chown-1

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   capabilities:
     - cap: cap_chown
       add: 1
```

#### config-capa-cap_setuid-0

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   capabilities:
     - cap: cap_setuid
       add: 0
```

#### config-capa-cap_setuid-1

```YAML
headers:
  alias: alias
  name: name
  info: info
config:
   capabilities:
     - cap: cap_setuid
       add: 1
```

#### environ-unset

```YAML
headers:
  alias: alias
  name: name
  info: info
environ:
  - key: VAR
    value: '$NODE_ALIAS($LEAF_ALIAS)> '
```

#### environ-default

```YAML
headers:
  alias: alias
  name: name
  info: info
environ:
  - key: VAR
    mode: default
    value: '$NODE_ALIAS($LEAF_ALIAS)> '
```

#### environ-static

```YAML
headers:
  alias: alias
  name: name
  info: info
environ:
  - key: VAR
    mode: static
    value: '$NODE_ALIAS($LEAF_ALIAS)> '
```

#### environ-execfd

```YAML
headers:
  alias: alias
  name: name
  info: info
environ:
  - key: VAR
    mode: execfd
    value: 'echo HELLO'
```

#### environ-remove

```YAML
headers:
  alias: alias
  name: name
  info: info
environ:
  - key: VAR
    mode: remove
```

#### export-restricted-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: restricted
    mount: /A
    path: /somewhere
```

#### export-restricted-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: restricted
    mount: /B
    path: /somewhere
```

#### export-public-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: public
    mount: /A
    path: /somewhere
```

#### export-public-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: public
    mount: /B
    path: /somewhere
```

#### export-private-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: private
    mount: /A
    path: /somewhere
```

#### export-private-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: private
    mount: /B
    path: /somewhere
```

#### export-privaterestricted-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: privaterestricted
    mount: /A
    path: /somewhere
```

#### export-privaterestricted-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: privaterestricted
    mount: /B
    path: /somewhere
```

#### export-restrictedfile-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: restrictedfile
    mount: /A
    path: /somewhere
```

#### export-restrictedfile-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: restrictedfile
    mount: /B
    path: /somewhere
```

#### export-publicfile-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: publicfile
    mount: /A
    path: /somewhere
```

#### export-publicfile-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: publicfile
    mount: /B
    path: /somewhere
```

#### export-privatefile-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: privatefile
    mount: /A
    path: /somewhere
```

#### export-privatefile-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: privatefile
    mount: /B
    path: /somewhere
```

#### export-symlink-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: symlink
    mount: /A
    path: /somewhere
```

#### export-symlink-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: symlink
    mount: /B
    path: /somewhere
```

#### export-internal-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: internal
    mount: /A
    path: /somewhere
```

#### export-internal-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: internal
    mount: /B
    path: /somewhere
```

#### export-execfd-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: execfd
    mount: /A
    path: 'echo /somewhere/A'
```

#### export-execfd-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: execfd
    mount: /B
    path: 'echo /somewhere/B'
```

#### export-anonymous-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: anonymous
    mount: /A
```

#### export-anonymous-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: anonymous
    mount: /B
```

#### export-tmpfs-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: tmpfs
    mount: /A
```

#### export-tmpfs-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: tmpfs
    mount: /B
```

#### export-procfs-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: procfs
    mount: /A
```

#### export-procfs-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: procfs
    mount: /B
```

#### export-devfs-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: devfs
    mount: /A
```

#### export-devfs-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: devfs
    mount: /B
```

#### export-mqueue-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: mqueue
    mount: /A
```

#### export-mqueue-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: mqueue
    mount: /B
```

#### export-lock-A

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: lock
    mount: /A
```

#### export-lock-B

```YAML
headers:
  alias: alias
  name: name
  info: info
exports:
  - mode: lock
    mount: /B
```

