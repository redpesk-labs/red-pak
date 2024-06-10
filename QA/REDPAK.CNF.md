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

| tag                 | type        | description                                                  |
|---------------------|-------------|--------------------------------------------------------------|
| NODE_PREFIX         | environment | ENVAL(NODE_PREFIX, )                                         |
| redpak_MAIN     (*) | environment | ENVAL(redpak_MAIN, $NODE_PREFIX/etc/redpak/main.yaml)        |
| redpak_TMPL     (*) | environment | ENVAL(redpak_TMPL, $NODE_PREFIX/etc/redpak/templates.d)      |
| REDNODE_CONF        | environment | ENVAL(REDNODE_CONF, $NODE_PATH/etc/redpack.yaml)             |
| REDNODE_ADMIN       | environment | ENVAL(REDNODE_ADMIN, $NODE_PATH//etc/redpak/main-admin.yaml) |
| REDNODE_STATUS      | environment | ENVAL(REDNODE_STATUS, $NODE_PATH/.rednode.yaml)              |
| REDNODE_VARDIR      | environment | ENVAL(REDNODE_VARDIR, $NODE_PATH/var/rpm/lib)                |
| REDNODE_REPODIR     | environment | ENVAL(REDNODE_REPODIR, $NODE_PATH/etc/yum.repos.d)           |
| REDNODE_LOCK        | environment | ENVAL(REDNODE_LOCK, $NODE_PATH/.rpm.lock)                    |
| LOGNAME             | environment | ENVAL(LOGNAME, Unknown)                                      |
| HOSTNAME            | environment | ENVAL(HOSTNAME, localhost)                                   |
| CGROUPS_MOUNT_POINT | environment | ENVAL(CGROUPS_MOUNT_POINT, /sys/fs/cgroup)                   |
| LEAF_ALIAS          | environment | ENVAL(LEAF_ALIAS, #undef)                                    |
| LEAF_NAME           | environment | ENVAL(LEAF_NAME, #undef)                                     |
| LEAF_PATH           | environment | ENVAL(LEAF_PATH, #undef)                                     |
| PID                 | pid         | the PID                                                      |
| UID                 | uid         | the UID                                                      |
| GID                 | gid         | the GID                                                      |
| TODAY               | date        | current date and hour                                        |
| UUID                | uuid        | new UUID                                                     |
| NODE_ALIAS          | automatic   | the node alias                                               |
| NODE_NAME           | automatic   | the node name                                                |
| NODE_PATH           | automatic   | the node path                                                |
| NODE_INFO           | automatic   | the node info                                                |
| REDPESK_VERSION     | environment | ENVAL(REDPESK_VERSION, agl-redpesk9)                         |

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
| *headers*     | mandatory | map      | meta data          |
| *config*      | optional  | map      | settings           |
| *environ*     | optional  | sequence | exported variables |
| *relocations* | optional  | sequence | renamed files      |
| *exports*     | optional  | sequence | exported files     |

It is an error if the *header* entry is absent.

### Valid headers map

.RULE

The main entry *headers* must be a valid map with valid content as described below:

| name          | presence  | type   | description             |
|---------------|-----------|--------|-------------------------|
| *alias*       | mandatory | string | name of the *rednode*   |
| *name*        | optional  | string | ?                       |
| *info*        | optional  | string | auto documentation text |

It is an error if the *alias* entry is absent.

It is an error if the value of *alias* is invalid.

### Valid alias

.RULE

????? any string

TODO: validate alias name using strict rules

### Valid environ sequence

.RULE

The main entry *environ* must be a valid sequence of valid environ entries

### Valid environ entries

.RULE

Valid environ entries are maps with the entries described below:

| name          | presence  | type   | description                             |
|---------------|-----------|--------|-----------------------------------------|
| *mode*        | optional  | enum   | mode, default to "default"              |
| *key*         | mandatory | string | name of the variable                    |
| *value*       | depends   | string | value depending on the mode             |
| *info*        | optional  | string | auto documentation text                 |
| *warn*        | optional  | string | warning message on overwriting in child |


It is an error if the *key* entry is absent.

It is an error if the *value* entry is absent when the mode requires it.

It is an error if the *value* entry is present when the mode does not expect it.

TODO: check processing of letter's case

### Valid  mode of environ entries

.RULE

Valid mode for environ entries are:

| mode      | value    | description                             |
|-----------|----------|-----------------------------------------|
| "Remove"  | no       | value depending on the mode             |
| "Static"  | required | the variable is set with the value      |
| "Default" | required | the variable is set with the value after expansion |
| "Execfd"  | required | the value is a shell command, the variable is set with the output file  of that command |

It is an error is the mode is not one of the above value.

### Valid relocations sequence

.RULE

The main entry *relocations* must be a valid sequence of valid relocation entries.


### Valid relocation entries

.RULE

Valid relocation entries are maps with the entries described below:

| name  | presence  | type   | description                             |
|-------|-----------|--------|-----------------------------------------|
| *old* | mandatory | string | name of the variable                    |
| *new* | mandatory | string | value depending on the mode             |

TODO: check processing of letter's case

### Valid exports sequence

.RULE

The main entry *exports* must be a valid sequence of valid export entries.


### Valid export entries

.RULE

Valid export entries are maps with the entries described below:

| name    | presence  | type   | description                             |
|---------|-----------|--------|-----------------------------------------|
| *mode*  | mandatory | enum   | mode, default to "default"              |
| *mount* | mandatory | string | name of the variable                    |
| *path*  | depends   | string | value depending on the mode             |
| *info*  | optional  | string | auto documentation text                 |
| *warn*  | optional  | string | warning message on overwriting in child |

It is an error if the *mode* entry is absent.

It is an error if the *mount* entry is absent.

It is an error if the *path* entry is absent when the mode requires it.

It is an error if the *path* entry is present when the mode does not expect it.


### Valid  mode of export entries

.RULE

Valid mode for export entries are:

| mode                | path     | description                             |
|---------------------|----------|-----------------------------------------|
| "Restricted"        | optional | value depending on the mode             |
| "Public"            | optional | the variable is set with the value      |
| "Private"           | optional | the variable is set with the value      |
| "PrivateRestricted" | optional | the variable is set with the value      |
| "RestrictedFile"    | optional | the variable is set with the value      |
| "PublicFile"        | optional | the variable is set with the value      |
| "PrivateFile"       | optional | the variable is set with the value      |
| "Anonymous"         | avoid    | the variable is set with the value      |
| "Symlink"           | optional | the variable is set with the value      |
| "Execfd"            | optional | the variable is set with the value      |
| "Internal"          | optional | the variable is set with the value      |
| "Tmpfs"             | avoid    | the variable is set with the value      |
| "Procfs"            | avoid    | the variable is set with the value      |
| "Mqueue"            | avoid    | the variable is set with the value      |
| "Devfs"             | avoid    | the variable is set with the value      |
| "Lock"              | avoid    | the variable is set with the value      |

TODO: rules must be better set, atm mount is copied as path if path is missing
but it not always coherent. Also here, avoid is using fact that path is expanded(mount)


### Valid config map

.RULE

The main entry *config* must be a valid map containing any number of the below
entries, all are optional:

| name            | type     | description                                |
|-----------------|----------|--------------------------------------------|
| persistdir      | string   | (path) location of dnf database            |
| rpmdir          | string   | (path) location of rpm database            |
| cachedir        | string   | (path) location of dnf cache               |
| path            | string   | (path list) program search path list       |
| ldpath          | string   | (path list) library search path list       |
| umask           | string   | octal umask value                          |
| verbose         | int      | verbosity level                            |
| maxage          | int      | ?                                          |
| map-root-user   | int      | set user as root                           |
| gpgcheck        | boolean  | check rpm signature and gpg                |
| inherit         | boolean  | ?                                          |
| unsafe          | boolean  | remove safety checks on date and inode     |
| die-with-parent | EDU      | if enable terminate when parent terminates |
| new-session     | EDU      | if enable create a new session             |
| share_all       | EDU      | if enabled unshare all namespaces          |
| share_user      | EDU      | if enabled unshare user namespaces         |
| share_cgroup    | EDU      | if enabled unshare cgroup namespaces       |
| share_net       | EDU      | if enabled unshare network namespaces      |
| share_pid       | EDU      | if enabled unshare PID namespaces          |
| share_ipc       | EDU      | if enabled unshare IPC namespaces          |
| share_time      | EDU      | if enabled unshare UTS namespaces          |
| cgroups         | map      | description of cgroups                     |
| hostname        | string   | set the hostname                           |
| chdir           | string   | set execution directory                    |
| cgrouproot      | string   | controling cgroup root                     |
| capabilities    | sequence | list of capability's entries               |

The type EDU is an enumeration, so a string, with 3 possible values:
"enable", "disable", "unset".

TODO: review where strings are for paths

TODO: validation of paths? umask? ...

TODO: check real type of map-root-user, int or boolean?

TODO: remove maxage, inherit that are not used

TODO: default value of gpgcheck is not true

TODO: check if new-session, die-with-parent are boolean or not

TODO: what to do of the mix between kebab (like new-session) case and snake case (like share_all)

TODO: EDU mostly is on/off but unset has some meaning to, it compensate the lack of unset feature of libcyaml

### Valid cgroups map

.RULE

Within main *config* map, the map *cgroups* must have
the below entries, all are optional:

| name   | type   | description                                |
|--------|--------|--------------------------------------------|
| cpuset | cpuset | CPU attributions
| mem    | mem    | memory limitations
| cpu    | cpu    | CPU limitations
| io     | io     | I/O limitations
| pids   | pids   | 

### Valid cpuset map of cgroups

.RULE

### Valid mem map of cgroups

.RULE

### Valid cpu map of cgroups

.RULE

### Valid io map of cgroups

.RULE

### Valid pids map of cgroups

.RULE



## Evaluation

### Evaluation of environ entry mode remove

.RULE

### Evaluation of environ entry mode static

.RULE

### Evaluation of environ entry mode default

.RULE

### Evaluation of environ entry mode execfd

.RULE

### Evaluation of exports entry mode Restricted

.RULE

### Evaluation of exports entry mode Public

.RULE

### Evaluation of exports entry mode Private

.RULE

### Evaluation of exports entry mode PrivateRestricted

.RULE

### Evaluation of exports entry mode RestrictedFile

.RULE

### Evaluation of exports entry mode PublicFile

.RULE

### Evaluation of exports entry mode PrivateFile

.RULE

### Evaluation of exports entry mode Anonymous

.RULE

### Evaluation of exports entry mode Symlink

.RULE

### Evaluation of exports entry mode Execfd

.RULE

### Evaluation of exports entry mode Internal

.RULE

### Evaluation of exports entry mode Tmpfs

.RULE

### Evaluation of exports entry mode Procfs

.RULE

### Evaluation of exports entry mode Mqueue

.RULE

### Evaluation of exports entry mode Devfs

.RULE

### Evaluation of exports entry mode Lock

.RULE



















# Config yaml file good practices

The purpose of this part is to give advices to write a config yaml file to
configure your nodes within the hierarchical view.

There are 4 parts in node config, and default yaml templates
can be seen here: [Configuration]({% chapter_link redpak.configuration %}) chapter.

Note that a node is totally manageable through the yaml config files and
so directly represents the dynamic node behavior.

- [Config yaml file good practices](#config-yaml-file-good-practices)
  - [Redconf tool](#redconf-tool)
  - [Config special variables](#config-special-variables)
    - [Example](#example)
      - [First case from the parent point of view](#first-case-from-the-parent-point-of-view)
      - [Second case from the child point of view](#second-case-from-the-child-point-of-view)
  - [Headers part](#headers-part)
  - [Config part](#config-part)
    - [Namespaces](#namespaces)
    - [Capabilities](#capabilities)
    - [Cgroups](#cgroups)
  - [Export part](#export-part)
    - [Export modes](#export-modes)
    - [Export hierarchical constraints](#export-hierarchical-constraints)
  - [Environ part](#environ-part)
    - [Environ hierarchical constraints](#environ-hierarchical-constraints)

## Redconf tool

`redconf` tool is a tool that will parse and provide information about a node yaml config file.

The 2 modes can be useful to fix grammar issues with `redconf config` or to see what overloaded or getting the cumulated/merged config of a node with `redconf mergeconfig`.

See [link_name]({% chapter_link redpak.usage-guide %}) section for more information about redconf.

## Config special variables

There are some particular variables used by redpak that defined proper redpak features.

- `LEAF_ALIAS`: the leaf/current node alias (e.g. child1)
- `LEAF_PATH`:  the leaf/current node path (/var/redpak/parent/child1)
- `NODE_ALIAS`: this is the alias of the node: matching node alias from config file (e.g. /var/redpak)
- `NODE_PATH`:  this is the path of the node: matching node path from config file (e.g. /var/redpak)

There are defined in config file as environment variables.

### Example

Here an example of nodes with a parent(/var/redpak/parent) and a child(var/redpak/parent/child).

A part of the yaml config parent node:

```yaml
# parent config yaml file
exports:
  - mount: /home/$LEAF_ALIAS/dir1
    path: $NODE_PATH/dir1
    mode: Restricted
  - mount: /nodes/$NODE_ALIAS/usr
    path: $NODE_PATH/usr
    mode: Restricted
```

```yaml
# child config yaml file
exports:
  - mount: /home/$LEAF_ALIAS/dir2
    path: $NODE_PATH/dir2
    mode: Restricted
  - mount: /nodes/$NODE_ALIAS/usr
    path: $NODE_PATH/usr
    mode: Restricted
```

#### First case from the parent point of view

Have a look at the expanded variable:

| key | value | config file |
| - | - | - |
| `LEAF_ALIAS` | parent | in all nodes |
| `LEAF_PATH` | /var/redpak/parent | in all nodes |
| `NODE_ALIAS` | parent | parent node |
| `NODE_PATH` | /var/redpak/parent | parent node |

Have a look at expanded result of the parent config file:

```bash
redconf mergeconfig -r /var/redpak/parent
```

```yaml
# common variables:
# LEAF_ALIAS=parent LEAF_PATH=/var/redpak/parent

# config from parent:
#       NODE_ALIAS=parent
#       NODE_PATH=/var/redpak/parent
- mount: /home/parent/dir1               # /home/$LEAF_ALIAS/dir1
  path: /var/redpak/parent/dir1         # $NODE_PATH/dir1
  mode: Restricted
- mount: /nodes/parent/usr              # /nodes/$NODE_ALIAS/usr
  path: /var/redpak/parent/usr          # $NODE_PATH/usr
  mode: Restricted
```

#### Second case from the child point of view

Have a look at the expanded variable:

| key | value | config file |
| - | - | - |
| `LEAF_ALIAS` | child | in all nodes |
| `LEAF_PATH` | /var/redpak/parent/child | in all nodes |
| `NODE_ALIAS` | parent | parent node |
| `NODE_PATH` | /var/redpak/parent | parent node |
| `NODE_ALIAS` | child | child node |
| `NODE_PATH` | /var/redpak/parent/child | child node |

Have a look at expanded result of the parent config file:

```bash
redconf mergeconfig -r /var/redpak/parent/child
```

```yaml
# common variables:
# LEAF_ALIAS=child LEAF_PATH=/var/redpak/parent/child

# config from parent:
#       NODE_ALIAS=parent
#       NODE_PATH=/var/redpak/parent
- mount: /home/child/dir1               # /home/$LEAF_ALIAS/dir1
  path: /var/redpak/parent/dir1         # $NODE_PATH/dir1
  mode: Restricted
- mount: /nodes/parent/usr              # /nodes/$NODE_ALIAS/usr
  path: /var/redpak/parent/usr          # $NODE_PATH/usr
  mode: Restricted

# config from child:
#       NODE_ALIAS=child
#       NODE_PATH=/var/redpak/parent/child
- mount: /home/child/dir2               # /home/$LEAF_ALIAS/dir2
  path: /var/redpak/parent/child/dir2   # $NODE_PATH/dir2
  mode: Restricted
- mount: /nodes/child/usr               # /nodes/$NODE_ALIAS/usr
  path: /var/redpak/parent/child/usr    # $NODE_PATH/usr
  mode: Restricted
```

## Headers part

The headers are automatically provided at the node creation, it mainly gives
information about the node.

You can see example in default yaml file.

## Config part

The config part defined constraints and mandatory values of the nodes.

Some values are cumulated as the `path` of `ldpath` other are overloaded by children,
typically some proper value of a node as `hostname`.

Eventually, some values cannot be overloaded, they defined security constraints as namespace values.

Here, the different constraint behavior is explained within the node hierarchy.

- [namespaces](#namespaces)
- [capabilities](#capabilities)
- [cgroups](#cgroups)

### Namespaces

redpak handles all linux namespaces, see man documentation: [https://man7.org/linux/man-pages/man7/namespaces.7.html](https://man7.org/linux/man-pages/man7/namespaces.7.html)

- share_user
- share_cgroup
- share_net
- share_pid
- share_ipc
- share_time

There are 3 states for these values:

- `disabled` : means that a new namespace is created and none of the children can share it
- `enabled` : means that the namespace is shared and children are allowed to disable it
- `unset` : means that the disabled/enabled is authorize in children (default is disabled: meaning that if all nodes in the family is `unset` the namespace is disabled)

### Capabilities

Capabilities can be dropped or add in redpak.

```yaml
capabilities:
- cap: cap_net_raw
  add: 1 # add capability
- cap: cap_dac_override
  add: 0 # drop capability
```

If a capability is dropped by a parent, a child cannot add it.

If a capability is added by a parent, a child can drop it.

### Cgroups

link to troubleshooting

Cgroup for redpak only works in v2 version.
All of the elements of redpak config file respects kernel definitions, please have a look at [https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html).

Cgroups already have a hierarchical behavior and so, for now there are no static ways to detect errors with cgroups.

But of course, children cannot bypass parent constraints in cgroups.

For example, if a parent set a max pid to 20 and a child to 50: the max pid in will be 20.

## Export part

The export part defines the mount points inside your node, it also inherits from parent export part configuration.

In main cases, a export is defined by a yaml map of 3 elements like below:

```yaml
- mount: /destination/mount/path/inside/node # the path inside the node
  path: /path/source/to/mount/inside/node # the path source in the system
  mode: Public # the mode
```

- [modes](#export-modes)
- [hierarchical constraints](#export-hierarchical-constraints)

### Export modes

Below the list of the different export modes in config file and with the hierarchical view:

| Mode | description | where | path
|-|-|-|-|
| `Public` | **rw** | current node / children | yes |
| `Restricted` | **ro** | current node / children | yes |
| `Private` | **rw** | current node (not in children) | yes |
| `PrivateRestricted` | **ro** | current node (not in children) | yes |
| `RestrictedFile` | **ro (file)** | current node / children | yes |
| `PublicFile` | **rw (file)** | current node / children | yes |
| `PrivateFile` | **rw (file)** | current node (not in children) | yes |
| `Anonymous` | **create dir at `mount`** | current node/children | no |
| `Symlink` | **create symlink from `path`(in node) to `mount`** | current node/children | yes |
| `Execfd` | **create file from shell cmd to `mount`** | current node/children | yes (shell command) |
| `Internal` | **copy file descriptor from `path` to `mount`** | current node/children | yes (file descriptor) |
| `Tmpfs` | **new tmpfs on `mount`** | current node/children | no |
| `Procfs` | **new procfs on `mount`** | current node/children | no |
| `Mqueue` | **new mqueue on `mount`** | current node/children | no |
| `Devfs` | **new dev on `mount`** | current node/children | no |
| `Lock` | **take a lock on `mount`** | current node/children | no |

### Export hierarchical constraints

The hierarchical constraints for exports are quite simple. All redundant mounts are ignored by redpak. That is to say, a child cannot mount anything at the same destination.

Thus, a child should never have a same destination mount path in its configuration. `redconf mergeconfig` can help to check such information. See [link_name]({% chapter_link redpak.usage-guide %}) for more detail.

Example:

```yaml
# parent configuration
- mode: Restricted
  mount: /var/lib/rpm
  path: /var/parent/rpm
```

```yaml
# child configuration
- mode: Public
  mount: /var/lib/rpm
  path: /var/parent/child/rpm
```

In this case, `/var/lib/rpm` will be mounted from the parent configuration with `Restricted` from `/var/parent/rpm`.

See `redconf mergeconfig` warning:

```bash
redconf mergeconfig --redpath /var/parent/child
```

```yaml
- mode: Public
  mount: /var/lib/rpm
  path: /var/parent/child/rpm
  info: /var/parent/parent/child
  warn: key=/var/lib/rpm is overload by /var/parent
```

## Environ part

The environ part defines environment variables inside node.

```yaml
environ:
  - key: PS1
    value: Rednode($LEAF_ALIAS)>
    mode: Default
```

Here below the different modes:

- Default: expand $VAR at runtime
- Static:  use value without expanding variables
- Execfd: User bash command STDOUT as var value
- Remove: Skim existing environment

### Environ hierarchical constraints

The constraints are similar to export constraints, a environment variable cannot be overload by a child.

```yaml
#  parent
environ:
  - key: MYKEY
    value: THISISMYKEYFROMPARENT
    mode: Default
```

```yaml
#  parent
environ:
  - key: MYKEY
    value: THISISMYKEYFROMCHILD
    mode: Default
```

In this case, `MYKEY` will be set from the parent configuration with THISISMYKEYFROMCHILD.

See `redconf mergeconfig` warning:

```bash
redconf mergeconfig --redpath /var/parent/child
```

```yaml
- mode: Default
  key: MYKEY
  value: THISISMYKEYFROMCHILD
  info: /var/redpak/parent/child
  warn: key=MYKEY is overload by /var/redpak/parent/child
```

## APPENDIX A - example of configuration

```yaml
#---------------------------------------------------------------------------
#
#               redpak CoreOs config
#
# - Author: Fulup Ar Foll (fulup@iot.bzh)
# - Author: Clément Bénier (clement.benier@iot.bzh)
# - Define default exec time values for every rednode nspace
# - Syntax is identical for all rednode (root, platform, terminal, ...)
#
# - RedNode redpack.yaml is typically generated at creation time from template:
#   + redwrap-dnf --redpath=/xxxx/xxxx --alias=xxxx [--template=default]
#-----------------------------------------------------------------------------


#---------------------------------------------------------------------
# Headers: Automtically generated at the node creation
#   Alias: alias of the node given by --alias at the creation or the basename of redpath
#   Name : generated uuid
#   Info: information of user/date
#
# below an header example
#---------------------------------------------------------------------
headers:
  alias: parent
  name: 77460e61-2dd7-4122-845f-4a644e01f73b
  info: Node created by rp-owner(localhost.localdomain) the 16-May-2022 May:29 (CEST)

#---------------------------------------------------------------------
# Config:
#   In this section tag can be defined at any level of the node family
#    - root (coreos) value might be overloaded
#    - if multiple definition the oldest ancestor wins
#---------------------------------------------------------------------
config:

    # dnf/rpm config value from current node is used
    rpmdir: /var/lib/rpm
    persistdir: /var/lib/dnf
    cachedir: $NODE_PATH/var/cache/dnf
    gpgcheck: true
    verbose: 0
    maxage: 0
    umask: 027

    # value are cumulated (coreos 1st, then node family from the yougest to the oldest)
    path: '/bin'
    ldpath: '/lib64'

    # nspace share/unshare option (oldest ancesor win)
    # - unset: ignored
    # - enabled: use share-xxx
    # - disabled: use unshare-xxx
    share_all: disabled
    share_user: unset
    share_cgroup: unset
    share_net: enabled
    share_pid: unset
    share_ipc: unset
    share_time: unset

    # add/drop capabilities
    # capabilities:
    #   - cap: CAP_SHOWN
    #     add: 1 # add capability
    #   - cap: CAP_SHOWN
    #     add: 0 # drop capability

    # Change HOSTNAME and move to prefere directory inside nspace
    hostname: $LEAF_ALIAS
    chdir: /home/$LEAF_ALIAS

    # nspace command control
    die-with-parent: enabled
    new-session: unset
    cgrouproot: /sys/fs/cgroup/user.slice/user-$UID.slice/user@$UID.service

    #---------------------------------------------------------------------
    # cgroups v2:
    # https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
    # cgroups:
    #         cpuset:
    #               cpus: 0-2
    #               mems: 0
    #               cpus_partition: member
    #         mem:
    #                 min: 10M
    #                 low: 10M
    #                 high: 521M
    #                 max: 512M
    #                 oom_group: 10
    #                 swap_high: 1024M
    #                 swap_max: 512M
    #         cpu:
    #                 max: 1000 1000
    #                 weight: 200
    #                 weight_nice: 10
    #         io:
    #               weight: 200
    #               max: 8:0 rbps=2097152 wiops=120
    #         pids:
    #               max: 50
    #---------------------------------------------------------------------

#---------------------------------------------------------------------
# Enviroment might be add or remove. Mode:
#  - Default: expand $VAR at runtime
#  - Static:  use value without expanding variables
#  - Execfd: User bash command STDOUT as var value
#  - Remove: Skim existing (coreos) environment
#
# Environement section from all rednode family cummulate
#---------------------------------------------------------------------
environ:
  - key: PS1
    value: Rednode($LEAF_ALIAS)>
    mode: Default

  - key: SHELL_SESSION_ID
    mode: Remove

  - key: HOME
    value: /home/$LEAF_ALIAS


#---------------------------------------------------------------------
# mode:
#  - Private: mount are visible from every apps running from a given node
#  - Public: share are RW for every child
#  - Restricted: share are RO for every child
#  - Anonymous: file are only visible from app within the same node+nspace
#  - Symlink: Create a private symbolic link within nspace
#  - Execfd: Virtual file added to nspace from bash command stdout
#  - Special Linux FileSystems
#    + Procfs
#    + Devfs
#    + Tmpsfs
#    + Lock
#  mount:
#    the mounting point within nspace
#  path:
#    the realpath on coreos

# Export section from all rednode family cummulate
#---------------------------------------------------------------------
exports:
  # systemd notify
  - mode: Public
    mount: /run/systemd/notify
    path: /run/systemd/notify

  # afb-binder sockets
  - mode: Public
    mount: /run/platform
    path: /run/platform

  # afb-binder sockets
  - mode: Public
    mount: /run/user/$UID
    path: /run/user/$UID

  # afb-binder workdir
  - mode: Public
    mount: /home/$UID/app-data
    path: /home/$UID/app-data


  # mount /usr/lib + /usr/bin in node '/' in order to keep /usr free for node hosted RPM
  - mount: /lib64
    path: /usr/lib64
    mode: Restricted

  - mount: /lib
    path: /usr/lib
    mode: Restricted

  - mount: /bin
    path: /usr/bin
    mode: Restricted

  # when network is exported, we probably need reesolv.conf
  - mount: /etc/resolv.conf
    path: /etc/resolv.conf
    mode: Restricted

  - mount: /home/$LEAF_ALIAS
    path: /nodes/_private
    mode: Symlink

  # /var and /var are anonymous and private to each application
  - mount: /var
    mode: Anonymous

  # create a fake /etc/passwd + group
  - mount: /etc/passwd
    path: getent passwd $UID 65534
    mode: Execfd

  - mount: /etc/group
    path: getent group $(id -g) 65534
    mode: Execfd

  # Exports coreos systemFS or fake with Anonymous/Tmpfs
  - mount: /proc
    mode: Procfs

  - mount: /dev
    mode: Devfs

  - mount: /tmp
    mode: Tmpfs

  - mount: /run
    mode: Anonymous

  # need for rpm -qa to work within node
  - mount: /usr/lib/rpm
    path: /lib/rpm
    mode: Symlink

  # needed for bash readline to work
  - mount: /usr/share
    path: /usr/share
    mode: Restricted

  # need a hard link on /usr/lib/rpm/rpmrc for rpm -qa


# private is here only for documentation
# ----------------------------------------------------
#  - private are only mounted when configpath==redpath
#  - this never happen with coreos/main.yaml
#  - mount: /etc
#     path: /etc
#     mode:  Private

#   - mount: /usr/libexec
#     path: /usr/libexec
#     mode: Restricted

#   - mount:
#     path: /dev/mqueue
#     mode: Mqueue

#   - mount:
#     path: /var/tmp/lock-$PID
#     mode: Lock
```
