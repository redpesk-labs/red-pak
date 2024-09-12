# Rednode configuration

Rednode configuration files fully decribe a rednode.

However because rednodes can be nested together in a hierarchical
tree structure, the real excution environment of a rednode
also include configuration of the rednodes that it belongs to, recursively.

## Processing

### Valid YAML file

.RULE 

A rednode configuration file is a YAML encoded text file.
Being a YAML text file has implication, it must be valid.
This should be checked using YAML cheching tools.

Invalid YAML file must be rejected.

### Keys are not case sensitive

.RULE

When parsing rednode configuration, name of sections, keys of
associative maps and enumerated values are processed without
regards to the case used. So the key `Key` is the same that
`key` or `KEY`.

### Content is expanded

.RULE

Immediately after being parsed, values of configuration files
are expanded. The expansion process allows to add some templating
in configurations. It can use environment variables, automatic variables
(like node alias or parent's alias), dynamic variables (like current date)
and result of command execution.

### Content must be valid

.RULE

The content of the configuration file must be valid
for each rule given below in the section content.


### Unexpected content is not valid

.RULE

Any content not specified make the whole
configuration file invalid.

## Expansion

### Expansion occurs on strings

.RULE 

Expansion occurs on strings, not on numbers, boolean, or, enumerations.

TODO: a really good expansion should also expand to numbers or to enumerations.
I am not sure that it is needed here but it should at least be checked.
The reason is to search in integration with libcyaml.

### Pattern expansion character

.RULE

Expansion is the replacement of expension patterns by the value resulting
of evaluation of the replacement pattern. Within strings, patterns are starting by
the character *dollar* (`$`, of UNICODE point 36).

When the pattern expansion character is followed by an opening bracket
(`(`, UNICODE point 40) the replacement is a *command evaluation replacement*,
otherwise, the replacement is a *tag replacement*.

TODO: allow or not the pattern `${}`

### Escaping pattern expansion character

.RULE

The escaping character is the *backslash* (`\`, UNICODE point 92). When the 
pattern expansion character is immediatyely following the escaping character
there is no expansion and the escaping character is removed. In other words,
writing `\$` produces `$`.

TODO: The escaping character should also be escaped but it is not the case ATM.

### Expansion of command evaluation replacement

.RULE

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

TODO: is there any interest in allowing  brackets in commands?

TODO: how to protect from nasty commands? what is the environment of evaluation?

TODO: precise more when is the command invoked? each launch? at installation.

TODO: precise if and how output is trimmed

### Expansion of tag replacement

.RULE

When the pattern expansion character is not immediatly succeeded
by an opening bracket, this is a *tag replacement*. Any character
following the pattern expansion character and in the range of
UNICODE codepoints from 48 to 122 are interpreted as a tag.

The characters that are in *lower case* are translated to upper case
before being searched.

The tag is used for evaluation of the replacement.
The only valid tags are:

| tag                     | type        | description                                                  |
|-------------------------|-------------|--------------------------------------------------------------|
| `NODE_PREFIX`           | environment | ENVAL(NODE_PREFIX, )                                         |
| `redpak_MAIN`       (*) | environment | ENVAL(redpak_MAIN, $NODE_PREFIX/etc/redpak/main.yaml)        |
| `redpak_MAIN_ADMIN` (*) | environment | ENVAL(redpak_MAIN_ADMIN, $NODE_PREFIX/etc/redpak/main-admin.yaml) |
| `redpak_TMPL`       (*) | environment | ENVAL(redpak_TMPL, $NODE_PREFIX/etc/redpak/templates.d)      |
| `REDNODE_CONF`          | environment | ENVAL(REDNODE_CONF, $NODE_PATH/etc/redpak.yaml)              |
| `REDNODE_ADMIN`         | environment | ENVAL(REDNODE_ADMIN, $NODE_PATH/etc/redpak-admin.yaml)       |
| `REDNODE_STATUS`        | environment | ENVAL(REDNODE_STATUS, $NODE_PATH/.rednode.yaml)              |
| `REDNODE_VARDIR`        | environment | ENVAL(REDNODE_VARDIR, $NODE_PATH/var/rpm/lib)                |
| `REDNODE_REPODIR`       | environment | ENVAL(REDNODE_REPODIR, $NODE_PATH/etc/yum.repos.d)           |
| `REDNODE_LOCK`          | environment | ENVAL(REDNODE_LOCK, $NODE_PATH/.rpm.lock)                    |
| `LOGNAME`               | environment | ENVAL(LOGNAME, Unknown)                                      |
| `HOSTNAME`              | environment | ENVAL(HOSTNAME, localhost)                                   |
| `CGROUPS_MOUNT_POINT`   | environment | ENVAL(CGROUPS_MOUNT_POINT, /sys/fs/cgroup)                   |
| `LEAF_ALIAS`            | environment | ENVAL(LEAF_ALIAS, #undef)                                    |
| `LEAF_NAME`             | environment | ENVAL(LEAF_NAME, #undef)                                     |
| `LEAF_PATH`             | environment | ENVAL(LEAF_PATH, #undef)                                     |
| `PID`                   | pid         | the PID                                                      |
| `UID`                   | uid         | the UID                                                      |
| `GID`                   | gid         | the GID                                                      |
| `TODAY`                 | date        | current date and hour                                        |
| `UUID`                  | uuid        | new UUID                                                     |
| `NODE_ALIAS`            | automatic   | the node alias                                               |
| `NODE_NAME`             | automatic   | the node name                                                |
| `NODE_PATH`             | automatic   | the node path                                                |
| `NODE_INFO`             | automatic   | the node info                                                |
| `REDPESK_VERSION`       | environment | ENVAL(REDPESK_VERSION, agl-redpesk9)                         |

TODO: remove unexpected character from the accepted set

TODO (*): tags redpak_MAIN and redpak_TMPL or given in lower case and are then
excluded because of the current code.

TODO: check definition of REDNODE_ADMIN that has a double slash in the middle.

TODO: because command evaluation replacement is able to output environment
variables it may be safe to allow tags not predefined to be replacd by the
matching environment variable.


### Valid root map

.RULE

At the root of a *rednode* configuration file is a map.
The content of the root map is:

| name          | presence  | type     | description        |
|---------------|-----------|----------|--------------------|
| `headers`     | mandatory | map      | meta data          |
| `config`      | optional  | map      | settings           |
| `environ`     | optional  | sequence | exported variables |
| `exports`     | optional  | sequence | exported files     |

It is an error if the *header* entry is absent.

### Valid headers map

.RULE

The main entry *headers* must be a valid map with valid content as described below:

| name          | presence  | type   | description             |
|---------------|-----------|--------|-------------------------|
| `alias`       | mandatory | string | name of the *rednode*   |
| `name`        | optional  | string | ?                       |
| `info`        | optional  | string | auto documentation text |

It is an error if the *alias* entry is absent.

It is an error if the value of *alias* is invalid.

### Valid alias

.RULE

????? any string

TODO: validate alias name using strict rules (ex: should not have any '/', should not start with '.', ...)

### Valid environ sequence

.RULE

The main entry *environ* must be a valid sequence of valid environ entries

### Valid environ entries

.RULE

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

TODO: check processing of letter's case

### Valid  mode of environ entries

.RULE

Valid mode for environ entries are:

| mode      | value    | description                             |
|-----------|----------|-----------------------------------------|
| `Remove`  | no       | value depending on the mode             |
| `Static`  | required | the variable is set with the value      |
| `Default` | required | the variable is set with the value after expansion |
| `Execfd`  | required | the value is a shell command, the variable is set with the output file  of that command |

It is an error if the mode is not one of the above value.

### Valid exports sequence

.RULE

The main entry *exports* must be a valid sequence of valid export entries.


### Valid export entries

.RULE

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

.RULE

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

TODO: rules must be better set, ATM mount is copied as path if path is missing
but it is not always coherent.


### Valid config map

.RULE

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

TODO: check if some values are really used (persistdir, rpmdir, cachedir, ...)

TODO: review where strings are for paths

TODO: validation of paths? umask? ...

TODO: check real type of map-root-user, int or boolean?

TODO: default value of gpgcheck is not true

TODO: check if new-session, die-with-parent are boolean or not

TODO: what to do of the mix between kebab (like new-session) case and snake case (like share_all)

TODO: EDU mostly is on/off but unset has some meaning too (it means let the children the possibility to on/off themselves), it compensate the lack of unset feature of libcyaml

### Valid cgroups map

.RULE

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

.RULE

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name             | type   | description                                |
|------------------|--------|--------------------------------------------|
| `cpus`           | string | 
| `mems`           | string | 
| `cpus_partition` | string | 

TODO check cpuset.cpus.exclusive for cpuset.cpus.partition

### Valid mem map of cgroups

.RULE

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


TODO check that oom_group is not used

TODO check that low is not used

### Valid cpu map of cgroups

.RULE

Within map *cgroups* of *config*, the entry *cpu*  must have
the below entries, all are optional:

| name          | type   | description                                |
|---------------|--------|--------------------------------------------|
| `weight`      | string | 
| `weight_nice` | string | 
| `max`         | string | 

TODO check that weight_nice is not used

### Valid io map of cgroups

.RULE

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name         | type   | description                                |
|--------------|--------|--------------------------------------------|
| `cost_qos`   | string | 
| `cost_model` | string | 
| `weight`     | string | 
| `max`        | string | 

TODO check that cost_qos is not used

TODO check that cost_model is not used

### Valid pids map of cgroups

.RULE

Within map *cgroups* of *config*, the entry *cpuset*  must have
the below entries, all are optional:

| name   | type   | description                                |
|--------|--------|--------------------------------------------|
| `max`  | string | 



