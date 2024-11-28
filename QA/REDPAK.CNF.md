# REDNODE configuration

*REDNODE* configuration files fully decribe a *REDNODE*.

However because *REDNODE*s can be nested together in a hierarchical
tree structure, the real excution environment of a *REDNODE*
also include configuration of the *REDNODE*s that it belongs to, recursively.

## Notation

Configuration files are structured by the use of nested maps (or dictionnaries)
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
pattern expansion character is immediatyely following the escaping character
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
following the pattern expansion character and in the range of
UNICODE codepoints from 48 to 122 are interpreted as a tag.

The characters that are in *lower case* are translated to upper case
before being searched.

The tag is used for evaluation of the replacement.
The only valid tags are:

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

### Valid root map

.RULE REDPAK.CNF-U-VAL-ROO-MAP

At the root of a *REDNODE* configuration file is a map.
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

Any string is valid

### Valid environ sequence

.RULE REDPAK.CNF-U-VAL-ENV-SEQ

The main entry *environ* must be a valid sequence of valid environ entries

### Valid environ entries

.RULE REDPAK.CNF-U-VAL-ENV-ENT

Valid environ entries are maps with the entries described below:

| name          | presence  | type   | description                             |
|---------------|-----------|--------|-----------------------------------------|
| `mode`        | optional  | enum   | mode, default to "default"              |
| `key`         | mandatory | string | name of the variable                    |
| `value`       | depends   | string | value depending on the mode             |
| `info`        | optional  | string | auto documentation text                 |
| `warn`        | optional  | string | warning message on overwriting in child |


It is an error if the *key* entry is absent.

It is an error if the *value* entry is absent when the mode requires it.

It is an error if the *value* entry is present when the mode does not expect it.

### Valid  mode of environ entries

.RULE REDPAK.CNF-U-VAL-MOD-ENV-ENT

Valid mode for environ entries are:

| mode      | value    | description                             |
|-----------|----------|-----------------------------------------|
| `Remove`  | no       | value depending on the mode             |
| `Static`  | required | the variable is set with the value      |
| `Default` | required | the variable is set with the value after expansion |
| `Execfd`  | required | the value is a shell command, the variable is set with the output of that command |
| `Inherit` | no       | the variable is set from its value      |

It is an error if the mode is not one of the above value.

### Valid exports sequence

.RULE REDPAK.CNF-U-VAL-EXP-SEQ

The main entry *exports* must be a valid sequence of valid export entries.


### Valid export entries

.RULE REDPAK.CNF-U-VAL-EXP-ENT

Valid export entries are maps with the entries described below:

| name    | presence  | type   | description                             |
|---------|-----------|--------|-----------------------------------------|
| `mode`  | mandatory | enum   | mode, default to "default"              |
| `mount` | mandatory | string | mount destination in the node           |
| `path`  | depends   | string | value depending on the mode             |
| `info`  | optional  | string | auto documentation text                 |
| `warn`  | optional  | string | warning message on overwriting in child |

It is an error if the *mode* entry is absent.

It is an error if the *mount* entry is absent.

It is an error if the *path* entry is absent when the mode requires it.

It is an error if the *path* entry is present when the mode does not expect it.


### Valid  mode of export entries

.RULE REDPAK.CNF-U-VAL-MOD-EXP-ENT

Valid mode for export entries are:

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
| `Mqueue`            | ignored  |       |
| `Lock`              | ignored  |       |

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
| `verbose`         | int      | verbosity level                            |
| `map-root-user`   | int      | set user as root                           |
| `gpgcheck`        | boolean  | check rpm signature and gpg                |
| `unsafe`          | boolean  | remove safety checks on date and inode     |
| `die-with-parent` | EDU      | if enabled terminate when parent terminates |
| `new-session`     | EDU      | if enabled create a new terminal session   |
| `share_all`       | EDU      | if enabled unshare all namespaces          |
| `share_user`      | EDU      | if enabled unshare user namespaces         |
| `share_cgroup`    | EDU      | if enabled unshare cgroup namespaces       |
| `share_net`       | EDU      | if enabled unshare network namespaces      |
| `share_pid`       | EDU      | if enabled unshare PID namespaces          |
| `share_ipc`       | EDU      | if enabled unshare IPC namespaces          |
| `share_time`      | EDU      | if enabled unshare UTS namespaces          |
| `cgroups`         | map      | description of cgroups                     |
| `hostname`        | string   | set the hostname                           |
| `chdir`           | string   | set execution directory                    |
| `cgrouproot`      | string   | controling cgroup root                     |
| `capabilities`    | sequence | list of capability's entries               |

The type EDU is an enumeration, so a string, with 3 possible values:
"enable", "disable", "unset".

### Valid cgroups map

.RULE REDPAK.CNF-U-VAL-CGR-MAP

Within main *config* map, the map *cgroups* must have
the below entries, all are optional:

| name     | type   | description             |
|----------|--------|-------------------------|
| `cpuset` | cpuset | CPU attributions        |
| `mem`    | mem    | memory limitations      |
| `cpu`    | cpu    | CPU limitations         |
| `io`     | io     | I/O limitations         |
| `pids`   | pids   | pids limitations        |

### Valid cpuset map of cgroups

.RULE REDPAK.CNF-U-VAL-CPUSET-MAP-CGR

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name             | type   | description                                |
|------------------|--------|--------------------------------------------|
| `cpus`           | string | 
| `mems`           | string | 
| `cpus_partition` | string | 

### Valid mem map of cgroups

.RULE REDPAK.CNF-U-VAL-MEM-MAP-CGR

Within map *cgroups* of *config*, the entry *mem*  must have
the below entries, all are optional:

| name        | type   | description                                |
|-------------|--------|--------------------------------------------|
| `max`       | string | 
| `high`      | string | 
| `min`       | string | 
| `low`       | string | 
| `oom_group` | string | 
| `swap_high` | string | 
| `swap_max`  | string | 


### Valid cpu map of cgroups

.RULE REDPAK.CNF-U-VAL-CPU-MAP-CGR

Within map *cgroups* of *config*, the entry *cpu*  must have
the below entries, all are optional:

| name          | type   | description                                |
|---------------|--------|--------------------------------------------|
| `weight`      | string | 
| `weight_nice` | string | 
| `max`         | string | 

### Valid io map of cgroups

.RULE REDPAK.CNF-U-VAL-IO-MAP-CGR

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name         | type   | description                                |
|--------------|--------|--------------------------------------------|
| `cost_qos`   | string | 
| `cost_model` | string | 
| `weight`     | string | 
| `max`        | string | 

### Valid pids map of cgroups

.RULE REDPAK.CNF-U-VAL-PID-MAP-CGR

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name   | type   | description                                |
|--------|--------|--------------------------------------------|
| `max`  | string | 


### Valid capability entry

.RULE REDPAK.CNF-U-VAL-CAP-ENT

Within sequence *capabilitues* of *config*, a vamid capabilty entry
is a map having the below entries, all are optional:

| name   | presence  | type   | description                                |
|--------|-----------|--------|--------------------------------------------|
| `cap`  | mandatory | string | name of the capability to add or drop
| `add`  | mandatory | integer | 0 for droping the capability, 1 for keeping it
| `info` | optional  | string | informative text
| `warn` | optional  | string | warning message when 

The allowed 41 capability names are:
    cap_chown,
    cap_dac_override,
    cap_dac_read_search,
    cap_fowner,
    cap_fsetid,
    cap_kill,
    cap_setgid,
    cap_setuid,
    cap_setpcap,
    cap_linux_immutable,
    cap_net_bind_service,
    cap_net_broadcast,
    cap_net_admin,
    cap_net_raw,
    cap_ipc_lock,
    cap_ipc_owner,
    cap_sys_module,
    cap_sys_rawio,
    cap_sys_chroot,
    cap_sys_ptrace,
    cap_sys_pacct,
    cap_sys_admin,
    cap_sys_boot,
    cap_sys_nice,
    cap_sys_resource,
    cap_sys_time,
    cap_sys_tty_config,
    cap_mknod,
    cap_lease,
    cap_audit_write,
    cap_audit_control,
    cap_setfcap,
    cap_mac_override,
    cap_mac_admin,
    cap_syslog,
    cap_wake_alarm,
    cap_block_suspend,
    cap_audit_read,
    cap_perfmon,
    cap_bpf,
    cap_checkpoint_restore

