# Config yaml file good practices

The purpose of this part is to give advices to write a config yaml file to
configure your nodes within the hierarchical view.

There are 4 parts in node config, and default yaml templates
can be seen here: [Configuration]({% chapter_link redpak.configuration %})

Note that a node is totally manageable through the yaml config files and
so directly represents the dynamic node behavior.

* [redconf tool](#redconf-tool)
* [Special variables](#config-special-variables)
  * [Example](#example)
    * [Parent point of view](#first-case-from-the-parent-point-of-view)
    * [Child point of view](#second-case-from-the-child-point-of-view)
* [Header part](#headers-part)
* [Config part](#config-part)
  * [namespaces](#namespaces)
  * [capabilities](#capabilities)
  * [cgroups](#cgroups)
* [Export part](#export-part)
  * [modes](#export-modes)
  * [hierarchical constraints](#hierarchical-contraints)
* [Environ part](#environ-part)

## Redconf tool

`redconf` tool is a tool that will parse and provide information about a node yaml config file.

The 2 modes can be useful to fix grammar issues with `redconf config` or to see what overloaded or getting the cumulated/merged config of a node with `redconf mergeconfig`.

See [link_name]({% chapter_link redpak.usage-guide %}) section for more information about redconf.

## Config special variables

There are some particular variables used by red-pak that defined proper red-pak features.

* `LEAF_ALIAS` : the leaf/current node alias (e.g. child1)
* `LEAF_PATH`: the leaf/current node path (/var/redpak/parent/child1)
* `NODE_ALIAS`: this is the alias of the node: matching node alias from config file (e.g. /var/redpak)
* `NODE_PATH`: this is the path of the node: matching node path from config file (e.g. /var/redpak)

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
typacally some proper value of a node as `hostname`.

Eventually, some values cannot be overloaded, they defined security constraints as namespace values.

Here, the different constraint behavior is explained within the node hierarchy.

* [namespaces](#namespaces)
* [capabilities](#capabilities)
* [cgroups](#cgroups)

### Namespaces

redpak handles all linux namespaces, see man documentation: [https://man7.org/linux/man-pages/man7/namespaces.7.html](https://man7.org/linux/man-pages/man7/namespaces.7.html)

* share_user
* share_cgroup
* share_net
* share_pid
* share_ipc
* share_time

There are 3 states for these values:

* `disabled` : means that a new namespace is created and none of the children can share it
* `enabled` : means that the namespace is shared and children are allowed to disable it
* `unset` : means that the disabled/enabled is authorize in children (default is disabled: meaning that if all nodes in the family is `unset` the namespace is disabled)

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
All of the elements of redpak config file respects kernel definitions, please have a look at https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html.

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

* [modes](#export-modes)
* [hierarchical constraints](#hierarchical-contraints)

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

### Hierarchical contraints

The hierarchical contraints for exports are quite simple. All redundant mounts are ignored by red-pak. That is to say, a child cannot mount anything at the same destination.

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

### Hierarchical constraints

The contraints are similar to export contraints, a environment variable cannot be overload by a child.

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
