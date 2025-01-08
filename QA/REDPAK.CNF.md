# REDNODE configuration

.VERSION: 2.4.1

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.git-id($Id$)

## Overview

For a short introduction to *REDPAK* and *REDNODE*s, see @REDPAK.OVE.

*REDNODE* configuration files fully describe a *REDNODE*.

However because *REDNODE*s can be nested in a hierarchical
tree structure, the real execution environment of a *REDNODE*
also depends on configuration of the *REDNODE*s that it belongs to.
The rule explaining how configuration of nested nodes is
computed are explained in  @REDPAK.DSG.

## Notation

Configuration files are structured by the use of nested maps (or dictionaries)
and list of values. In the remaining of that document, the entries
are referenced using common notation for path in such structured
data. This chapter details that notation.

In a map, the entries are referenced using dot. Then if `A`
is a map, `A.B` is the entry in `A` of name `B`.

The root of configurations is a map, then its entries
should be noted `<ROOT>.X` but for simplicity they are
merely noted `X` and the default root is implicit.

## Processing

### Valid YAML file

.RULE REDPAK.CNF-U-VAL-YAM-FIL

A *REDNODE* configuration file is a YAML encoded text file.

Invalid YAML file must be rejected.

Being a YAML text file has implication, it must be valid.
This should be checked using YAML checking tools.

### Keys are not case sensitive

.RULE REDPAK.CNF-U-KEY-ARE-NOT-CAS-SEN

When parsing *REDNODE* configuration, name of sections, keys of
associative maps and enumerated values are processed without
regards to the case used. So the key `Key` is the same that
`key` or `KEY`.

### Content is expanded

.RULE REDPAK.CNF-U-CON-EXP

Immediately after being parsed, values of configuration files
are expanded. The expansion process allows to add some templating
in configurations. It can use environment variables, automatic variables
(like node alias or parent's alias), dynamic variables (like current date)
and result of command execution.

### Content must be valid

.RULE REDPAK.CNF-U-CON-MUS-BE-VAL

The content of the configuration file must be valid
for each rule given below in the section content.

### Unexpected content is not valid

.RULE REDPAK.CNF-U-UNE-CON-NOT-VAL

Any content not specified make the whole
configuration file invalid.

## Expansion

### Pattern expansion

.RULE REDPAK.CNF-U-PAT-EXP-CHA

Expansion is the replacement of expansion patterns by the value resulting
of evaluation of the replacement pattern.

Expansion patterns are starting by the character *dollar* (`$`, of UNICODE point 36).
After the *dollar*, all character being letter, digit or underscore are also
part of the expansion pattern.

When the pattern expansion character is followed by an opening bracket
(`(`, UNICODE point 40) the replacement is a *command evaluation replacement*,
otherwise, the replacement is a *tag replacement*.

### Escaping pattern expansion character

.RULE REDPAK.CNF-U-ESC-PAT-EXP-CHA

The escaping character is the *backslash* (`\`, UNICODE point 92). When the
pattern expansion character is immediately following the escaping character
there is no expansion and the escaping character is removed. In other words,
writing `\$` produces `$`.

### Pattern expansion occurs on strings

.RULE REDPAK.CNF-U-EXP-OCC-STR

Expansion occurs on strings, not on numbers, boolean, or, enumerations.

### Expansion of command evaluation replacement

.RULE REDPAK.CNF-U-EXP-COM-EVA-REP

A *command evaluation replacement* is required when the pattern expansion
character is followed by an opening bracket (`(`, UNICODE point 40).
In that case, the opening bracket must be followed by a closing bracket
(`)`, UNICODE point 41) and any character between the 	brackets are
interpreted as a command to be evaluated by the standard shell. The standard
output of the evaluated command becomes the replacement string.

Example: the pattern `$(id -u nobody)` evaluate the shell command `id -u nobody`
that returns the numeric identifier of user *nobody*: `65534`.

Other example: the pattern `$(eval echo $VAR)` evaluate the shell command `eval echo $VAR`
that returns the value of the environment variable *VAR*.

### Expansion of tag replacement

.RULE REDPAK.CNF-U-EXP-TAG-REP

When the pattern expansion character is not immediately succeeded
by an opening bracket, this is a *tag replacement*. Any character
following the pattern expansion character and being either a letter
of any case (`[a-zA-Z]`), a digit (`[0-9]`) or the underscore (`_`)
is interpreted as a character of the tag.

The tag is used for evaluation of the replacement.

The below tags are predefined and reserved, they are not sensitive
to the case (mening that `$PID` could also be written `$Pid` or `$pid`):

| tag                   | type        | description                                                  |
|-----------------------|-------------|--------------------------------------------------------------|
| `NODE_PREFIX`         | environment | ENVAL(NODE_PREFIX, )                                         |
| `redpak_MAIN`         | environment | ENVAL(redpak_MAIN, $NODE_PREFIX/etc/redpak/main.yaml)        |
| `redpak_MAIN_ADMIN`   | environment | ENVAL(redpak_MAIN_ADMIN, \$NODE_PREFIX/etc/redpak/main-admin.yaml) |
| `redpak_TMPL`         | expand      | EXPVAL(\$NODE_PREFIX/etc/redpak/templates.d)                 |
| `REDNODE_CONF`        | expand      | EXPVAL(\$NODE_PATH/etc/redpak.yaml)                          |
| `REDNODE_ADMIN`       | expand      | EXPVAL(\$NODE_PATH/etc/redpak-admin.yaml)                    |
| `REDNODE_STATUS`      | expand      | EXPVAL(\$NODE_PATH/.*REDNODE*.yaml)                          |
| `REDNODE_VARDIR`      | expand      | EXPVAL(\$NODE_PATH/var/rpm/lib)                              |
| `REDNODE_REPODIR`     | expand      | EXPVAL(\$NODE_PATH/etc/yum.repos.d)                          |
| `REDNODE_LOCK`        | expand      | EXPVAL(\$NODE_PATH/.rpm.lock)                                |
| `LOGNAME`             | environment | ENVAL(LOGNAME, Unknown)                                      |
| `HOSTNAME`            | environment | ENVAL(HOSTNAME, localhost)                                   |
| `CGROUPS_MOUNT_POINT` | cgroup      | CGROUP(CGROUPS_MOUNT_POINT, MOUNTED, /sys/fs/cgroup)         |
| `PID`                 | pid         | current PID                                                  |
| `UID`                 | uid         | current UID                                                  |
| `GID`                 | gid         | current GID                                                  |
| `TODAY`               | date        | current date and hour                                        |
| `UUID`                | uuid        | new UUID                                                     |
| `LEAF_ALIAS`          | automatic   | the leaf alias                                               |
| `LEAF_NAME`           | automatic   | the leaf name                                                |
| `LEAF_PATH`           | automatic   | the leaf path                                                |
| `LEAF_INFO`           | automatic   | the leaf info                                                |
| `NODE_ALIAS`          | automatic   | the node alias                                               |
| `NODE_NAME`           | automatic   | the node name                                                |
| `NODE_PATH`           | automatic   | the node path                                                |
| `NODE_INFO`           | automatic   | the node info                                                |
| `REDPESK_VERSION`     | environment | ENVAL(REDPESK_VERSION, agl-redpesk9)                         |

If a tag is not one of the above predefined tags, since version 2.4.0,
the current environ is searched for an entry whose key matches exactly
the tag (case counts). If found, the value of this entry is substituted.

In all other cases, an empty string is substituted.


### Valid ROOT map

.RULE REDPAK.CNF-U-VAL-ROO-MAP

At its root, a *REDNODE* configuration file is a map.
The content of the root map is:

| name          | presence  | type     | description        |
|---------------|-----------|----------|--------------------|
| `headers`     | mandatory | map      | meta data          |
| `config`      | optional  | map      | settings           |
| `environ`     | optional  | sequence | exported variables |
| `exports`     | optional  | sequence | exported files     |

It is an error if the *header* entry is absent.

### Valid headers map

.RULE REDPAK.CNF-U-VAL-HEA-MAP

The main entry *headers* must be a valid map with valid content as described below:

| name          | presence  | type   | description             |
|---------------|-----------|--------|-------------------------|
| `alias`       | mandatory | string | name of the *REDNODE*   |
| `name`        | optional  | string | set automatically during creation of *REDNODE* |
| `info`        | optional  | string | documentation text |

It is an error if the *alias* entry is absent.

It is an error if the value of *alias* is invalid.

### Valid alias

.RULE REDPAK.CNF-U-VAL-ALI

An alias is a not empty string made only of character alphabetic, numeric,
or the symbols plus (+), minus (-), underscore (_), at-sign (@), colon (:).

**MOTIVATION** the characters slash, dollar, dot, backslash, number sign
and percent sign are used for special purpose in filesystem, shell and programming l

### Valid environ sequence

.RULE REDPAK.CNF-U-VAL-ENV-SEQ

The main entry *environ* must be a valid sequence of valid environ entries

### Valid environ entries

.RULE REDPAK.CNF-U-VAL-ENV-ENT

Valid *environ.\** entries are maps with the entries described below:

| name          | presence  | type   | description                             |
|---------------|-----------|--------|-----------------------------------------|
| `mode`        | optional  | enum   | mode, default to "default"              |
| `key`         | mandatory | string | name of the variable                    |
| `value`       | depends   | string | value depending on the mode             |
| `info`        | optional  | string | auto documentation text                 |
| `warn`        | optional  | string | warning message on overwriting in child |

It is an error if the *mode* is present but invalid.

It is an error if the *key* entry is absent.

It is an error if the *value* entry is absent when the mode requires it.

It is an error if the *value* entry is present when the mode does not expect it.

### Valid  mode of environ entries

.RULE REDPAK.CNF-U-VAL-MOD-ENV-ENT

Valid *environ.\*.mode* values are:

| mode      | value    | description                             |
|-----------|----------|-----------------------------------------|
| `Remove`  | no       | value depending on the mode             |
| `Static`  | required | the variable is set with the value      |
| `Default` | required | the variable is set with the value after expansion |
| `Execfd`  | required | the value is a shell command, the variable is set with the output of that command |
| `Inherit` | no       | the variable is set from its value inherited of the environment |

It is an error if the mode is not one of the above value.

### Valid exports sequence

.RULE REDPAK.CNF-U-VAL-EXP-SEQ

The main entry *exports* must be a valid sequence of valid export entries.


### Valid export entries

.RULE REDPAK.CNF-U-VAL-EXP-ENT

Valid *exports.\** entries are maps with the entries described below:

| name    | presence  | type   | description                             |
|---------|-----------|--------|-----------------------------------------|
| `mode`  | mandatory | enum   | mode, default to "default"              |
| `mount` | mandatory | string | mount destination in the node           |
| `path`  | depends   | string | value depending on the mode             |
| `info`  | optional  | string | auto documentation text                 |
| `warn`  | optional  | string | warning message on overwriting in child |

It is an error if the *mode* entry is absent.

It is an error if the *mode* is invalid.

It is an error if the *mount* entry is absent.

It is an error if the *path* entry is absent when the mode requires it.

It is an error if the *path* entry is present when the mode does not expect it.


### Valid  mode of export entries

.RULE REDPAK.CNF-U-VAL-MOD-EXP-ENT

Valid *exports.\*.mode* values are:

| mode                | path     | description                             |
|---------------------|----------|-----------------------------------------|
| `Restricted`        | optional | 'path' is a directory mounted in read-only at 'mount' |
| `Public`            | optional | 'path' is a directory mounted in read-write at 'mount' |
| `Private`           | optional | 'path' is a directory mounted in read-write at 'mount' only for current node |
| `PrivateRestricted` | optional | 'path' is a directory mounted in read-only at 'mount' only for current node |
| `RestrictedFile`    | optional | 'path' is a file mounted in read-only at 'mount' |
| `PublicFile`        | optional | 'path' is a file mounted in read-write at 'mount' |
| `PrivateFile`       | optional | 'path' is a file mounted in read-write at 'mount' only for current node |
| `Symlink`           | optional | make the symbolic link 'mount' pointing to 'path' |
| `Execfd`            | optional | execute the command 'path' and store its output in the file 'mount' |
| `Internal`          | optional | path is a file descriptor whose output is stored in the file 'mount' |
| `Anonymous`         | ignored  | creates the directory 'mount' |
| `Tmpfs`             | ignored  | mounts at 'mount' a new tmpfs file system |
| `Procfs`            | ignored  | mounts at 'mount' a new procfs file system |
| `Devfs`             | ignored  | mounts at 'mount' a new devfs file system |
| `Mqueue`            | ignored  | mounts at 'mount' a new devfs file system |
| `Lock`              | ignored  | locks the file 'mount' |

Being optional for a *path* entry means that if the value is not given,
the value of *mount* is used. This is a facility for reexporting items
with restriction.


### Valid config map

.RULE REDPAK.CNF-U-VAL-CON-MAP

The main entry *config* must be a valid map containing any number of the below
entries, all are optional:

| name              | type     | description                                |
|-------------------|----------|--------------------------------------------|
| `persistdir`      | string   | (path) location of dnf database            |
| `rpmdir`          | string   | (path) location of rpm database (not used!) |
| `cachedir`        | string   | (path) location of dnf cache               |
| `path`            | string   | (path list) program search path list       |
| `ldpath`          | string   | (path list) library search path list       |
| `umask`           | string   | octal umask value                          |
| `set-user`        | string   | set of process user before entering node   |
| `set-group`       | string   | set of process group before entering node  |
| `verbose`         | int      | verbosity level                            |
| `map-root-user`   | int      | set user as root                           |
| `gpgcheck`        | boolean  | check rpm signature and gpg                |
| `unsafe`          | boolean  | remove safety checks on date and inode     |
| `inherit-env`     | boolean  | automatic export of all environment        |
| `die-with-parent` | EDU      | if enabled terminate when parent terminates |
| `new-session`     | EDU      | if enabled create a new terminal session   |
| `share_all`       | EDU      | if enabled unshare all namespaces (except time) |
| `share_user`      | SHARE    | if enabled unshare user namespaces         |
| `share_cgroup`    | SHARE    | if enabled unshare cgroup namespaces       |
| `share_net`       | SHARE    | if enabled unshare network namespaces      |
| `share_pid`       | SHARE    | if enabled unshare PID namespaces          |
| `share_ipc`       | SHARE    | if enabled unshare IPC namespaces          |
| `share_time`      | SHARE    | if enabled unshare UTS namespaces          |
| `cgroups`         | map      | description of cgroups                     |
| `hostname`        | string   | set the hostname                           |
| `chdir`           | string   | set execution directory                    |
| `cgrouproot`      | string   | controlling cgroup root                    |
| `capabilities`    | sequence | list of capability's entries               |

The type EDU is an enumeration, so a string, with 3 possible values (case insensitive):
"enabled", "disabled", "unset".

The type SHARE is either an enumeration as EDU (3 possible values
"enabled", "disabled", "unset") but it can also be a path ot a name
space to be joined if allowed to do a such thing.

### Valid cgroups map

.RULE REDPAK.CNF-U-VAL-CGR-MAP

The map *config.cgroups* must have the below entries, all are optional,
all are map:

| name     | type   | description             |
|----------|--------|-------------------------|
| `cpuset` | cpuset | CPU attributions        |
| `mem`    | mem    | memory limitations      |
| `cpu`    | cpu    | CPU limitations         |
| `io`     | io     | I/O limitations         |
| `pids`   | pids   | pids limitations        |

Configuration content of cgroups entries is documented in
linux kernel documentation page [Control Group v2][1].

### Valid cpuset map of cgroups

.RULE REDPAK.CNF-U-VAL-CPUSET-MAP-CGR

The map *config.cgroups.cpuset*  must have at least one of the below entries, all are optional:

| name             | type   | description                                |
|------------------|--------|--------------------------------------------|
| `cpus`           | string | Lists of requested CPUs
| `mems`           | string | Lists of requested memory nodes
| `cpus_partition` | string | One of: member, root or isolated

### Valid mem map of cgroups

.RULE REDPAK.CNF-U-VAL-MEM-MAP-CGR

The map *config.cgroups.mem*  must have at least one of the below entries, all are optional:

| name        | type   | description                                |
|-------------|--------|--------------------------------------------|
| `min`       | string | Minimal memory allocated to the cgroup
| `max`       | string | Maximal memory over which OOM kill is sent
| `high`      | string | Memory usage throttle limit
| `swap_high` | string | Swap usage throttle limit
| `swap_max`  | string | Swap usage hard limit
| `low`       | string | Ignored (valid entry but no effect)
| `oom_group` | string | Ignored (valid entry but no effect)


### Valid cpu map of cgroups

.RULE REDPAK.CNF-U-VAL-CPU-MAP-CGR

The map *config.cgroups.cpu*  must have at least one of the below entries, all are optional:

| name          | type   | description                                |
|---------------|--------|--------------------------------------------|
| `weight`      | string | Schedule weight of the process
| `max`         | string | Maximum CPU bandwidth
| `weight_nice` | string | Ignored (use weight instead)

### Valid io map of cgroups

.RULE REDPAK.CNF-U-VAL-IO-MAP-CGR

The map *config.cgroups.io*  must have at least one of the below entries, all are optional:

| name         | type   | description                                |
|--------------|--------|--------------------------------------------|
| `weight`     | string | Weight of the I/O
| `max`        | string | BPS and IOPS based IO limit
| `cost_qos`   | string | Ignored (valid entry but no effect)
| `cost_model` | string | Ignored (valid entry but no effect)

### Valid pids map of cgroups

.RULE REDPAK.CNF-U-VAL-PID-MAP-CGR

The map *config.cgroups.pids*  must have one entry exactly:

| name   | type   | description                                |
|--------|--------|--------------------------------------------|
| `max`  | string | Hard limit of number of processes          |


### Valid capability entry

.RULE REDPAK.CNF-U-VAL-CAP-ENT

Within sequence *config.capabilities[*]*, a valid capability entry
is a map having the below entries:

| name   | presence  | type    | description                                |
|--------|-----------|---------|--------------------------------------------|
| `cap`  | mandatory | string  | name of the capability to add or drop
| `add`  | mandatory | integer | 0 for dropping the capability, 1 for keeping it
| `info` | optional  | string  | informative text
| `warn` | optional  | string  | warning message when

Beside the 41 capability names listed below, the special capability **ALL**
is also accepted for selecting all capabilities.

All capability names are insensitive to the case.


| name                     | description                                  |
|--------------------------|----------------------------------------------|
| *cap_audit_control*      | Enable and disable kernel auditing           |
| *cap_audit_read*         | Allow reading the audit                      |
| *cap_audit_write*        | Write records to kernel auditing             |
| *cap_block_suspend*      | Can block system suspend                     |
| *cap_bpf*                | Employ privileged BPF operations             |
| *cap_checkpoint_restore* | Checkpoint/restore functionality             |
| *cap_chown*              | Arbitrary changes to file UIDs and GIDs      |
| *cap_dac_override*       | Bypass file permission                       |
| *cap_dac_read_search*    | Bypass file read permission                  |
| *cap_fowner*             | Bypass  permission checks                    |
| *cap_fsetid*             | Don't clear set-user-ID and set-group-ID     |
| *cap_ipc_lock*           | Lock memory                                  |
| *cap_ipc_owner*          | Bypass permission checks on IPC objects      |
| *cap_kill*               | Bypass permission checks for sending signals |
| *cap_lease*              | Establish leases on arbitrary files          |
| *cap_linux_immutable*    | Set FS_APPEND_FL and FS_IMMUTABLE_FL         |
| *cap_mac_admin*          | Allow MAC configuration                      |
| *cap_mac_override*       | Override Mandatory Access Control            |
| *cap_mknod*              | Create special files                         |
| *cap_net_admin*          | Various network-related                      |
| *cap_net_bind_service*   | Bind socket to privileged ports              |
| *cap_net_broadcast*      | (Unused)                                     |
| *cap_net_raw*            | Use RAW and PACKET sockets                   |
| *cap_perfmon*            | Employ various performance-monitoring        |
| *cap_setfcap*            | Set arbitrary capabilities on a file         |
| *cap_setgid*             | Manipulations of process GIDs                |
| *cap_setpcap*            | Add any capability                           |
| *cap_setuid*             | Manipulations of process UIDs                |
| *cap_sys_admin*          | Many admin features                          |
| *cap_sys_boot*           | Use reboot                                   |
| *cap_sys_chroot*         | Use chroot, change mount namespaces          |
| *cap_sys_module*         | Load and unload kernel modules               |
| *cap_sys_nice*           | Scheduling and affinity                      |
| *cap_sys_pacct*          | Use acct                                     |
| *cap_sys_ptrace*         | Trace arbitrary processes                    |
| *cap_sys_rawio*          | Perform I/O port operations                  |
| *cap_sys_resource*       | Use reserved space                           |
| *cap_sys_time*           | Set system clock                             |
| *cap_sys_tty_config*     | Use vhangup and various privileged ioctl     |
| *cap_syslog*             | Perform privileged syslog                    |
| *cap_wake_alarm*         | Trigger system wake up                       |

[1]: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
