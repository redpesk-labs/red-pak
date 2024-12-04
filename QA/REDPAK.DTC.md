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
out of variable parts of the output).

#### Invalid YAML is rejected

.TEST-CASE  test-conf-1

Invalid YAML file is rejected.

.REQUIRED-BY @REDPAK.CNF-U-VAL-YAM-FIL

.REQUIRED-BY @REDPAK.DSG-R-RED-CON-FIL-ARE-YAM

#### Valid YAML is accepted

.TEST-CASE  test-conf-2

Valid YAML file is Accepted.

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

.REQUIRED-BY @REDPAK.CNF-U-KEY-ARE-NOT-CAS-SEN

#### Miss of required key is reported

.TEST-CASE  test-conf-4

The configuration parser detects that the key **header.alias**
misses and report it as an error.

.REQUIRED-BY @REDPAK.CNF-U-CON-MUS-BE-VAL

.REQUIRED-BY @REDPAK.CNF-U-UNE-CON-NOT-VAL

#### Miss of optional parts is valid

.TEST-CASE  test-conf-5

A minimal configuration is given, it must be accepted.

.REQUIRED-BY @REDPAK.CNF-U-CON-MUS-BE-VAL

#### Valid characters of alias

.TEST-CASE  test-conf-6

Valid characters of aliases must be accepted.

.REQUIRED-BY @REDPAK.CNF-U-VAL-ALI

#### Invalid characters of alias

.TEST-CASE  test-conf-7

Invalid characters of aliases are rejected.

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

If fully implemented, the composition matrix would be huge: 7225
entries. But because many of these settings are unrelated one with
another, it is possible to only check the subset of pairs of settings
requiring application of merge rules.

This leads to a total of 293 meaningful pairs.

See Appendice A for details.

#### Example of private export in hierarchy

.TEST-CASE export-private-A__export-private-A

.REQUIRED-BY @REDPAK.DSG-R-RED-LAY-HIE-FIL


### Live tests

.TEST-CASE REDPAK.DTC-T-LIV-TES

Test that *REDWRAP* isolates correctly applications.

.TYPE integration

#### Description

Live tests are done by running a program in a given configuration.

One live test is performed by applying the below steps:

1. setup the *REDNODE* using configuration files for the test
2. optionally compile the program that must run in *REDNODE*
3. optionally copy the program in *REDNODE* filesystem
4. run the program or the command in the setup *REDNODE*
   using *REDWRAP*.
5. compares the output with the expected validated reference.

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

A minimal configuration leads to a *REDNODE* containing nothing.
This is achieved by *REDWRAP/BWRAP* by entering an empty mount namespace.

But is there is really nothing then it can not execute any program.
For this reason, *busybox* is the only thing installed in the node.

.REQUIRED-BY @REDPAK.HRQ-R-RED-ALW-RED-FIL-ROO

.REQUIRED-BY @REDPAK.HRQ-R-RED-SET-ISO-PRO

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM

#### Variables are set and expanded

.TEST-CASE setting-of-variables

Simple test entering a node with nothing except busybox at root.
Defines 3 environment variables.
Displays the content and the environment.

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

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-ENT

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM

#### Can execute a script

.TEST-CASE execute-script

Simple test entering a node with busybox in /bin and program in testscript directory.
Defines the PATH environment variable and try to use it.
Displays the content and the environment.

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-ENV-ENT

#### Variable expansion

.TEST-CASE expansion-of-keywords

Simple test entering a node with nothing except busybox at root.
Check the values of expanded variables.
Displays the environment.

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-EXP-TAG-REP

.REQUIRED-BY @REDPAK.CNF-U-CON-EXP


#### Mounting directories, files, special file systems

.TEST-CASE basic-mounts

Simple test entering a node with busybox in /bin and program in testscript directory.
Mounts some basic export
Displays mounts and content of /bin

.REQUIRED-BY @REDPAK.CNF-U-VAL-EXP-ENT

.REQUIRED-BY @REDPAK.CNF-U-VAL-MOD-EXP-ENT

.REQUIRED-BY @REDPAK.HRQ-R-RED-MOU-SEL-PAR-HOS-FIL

.REQUIRED-BY @REDPAK.HRQ-R-RED-MOU-VOL-ARE-USI-TMP

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM


#### Memory coercion

.TEST-CASE mem-max

Check that setting config.cgroup.mem.max works

.REQUIRED-BY @REDPAK.CNF-U-VAL-MEM-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR


#### Pid coercion

.TEST-CASE pids-max

Check that setting config.cgroup.pids.max works

.REQUIRED-BY @REDPAK.CNF-U-VAL-PID-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR

#### CPU coercion

.TEST-CASE cpu-max

Check that setting config.cgroup.cpu.max works

.REQUIRED-BY @REDPAK.CNF-U-VAL-CPU-MAP-CGR

.REQUIRED-BY @REDPAK.HRQ-R-RED-LEV-PAR-CGR

#### Capabilities settings

.TEST-CASE capabilities1

Check first range of capabilities

.REQUIRED-BY @REDPAK.HRQ-R-RED-MAN-LIN-CAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CAP-ENT

.TEST-CASE capabilities2

Check second range of capabilities

.REQUIRED-BY @REDPAK.HRQ-R-RED-MAN-LIN-CAP

.REQUIRED-BY @REDPAK.CNF-U-VAL-CAP-ENT

## Appendice A - Basic test details

