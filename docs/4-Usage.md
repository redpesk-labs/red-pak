# Usage

## Presirequites on redpesk OS

Please have a look to [link_name]({% chapter_link quickstart.boot-a-redpesk-image %}) to boot a redpesk image.

```bash
# su root
dnf install red-pak
```

## Command line tools:

* [`redwrap`](#excute-application-with-redwrap-strace): **the container engine**: launch application inside nodes
* [`redwrap-dnf`](#install-a-package-with-redwrap-dnf): **package manager**: handle install/update/remove
* [`redconf`](#redconf): **visualise node config**: config/merge/tree

## Create rednodes

### Install a node (With a system node)

The command below create the parent node inside a system node

```bash
# su rp-owner
redwrap-dnf --redpah /var/redpak/parentnode manager --alias parentnode
```

```bash
# su rp-owner
redwrap-dnf --redpah /var/redpak/parentnode/childnode manager --alias childnode
```

### Install a node (Without system ndoe)

```bash
# su rp-owner
redwrap-dnf --redpah /var/redpak/parentnode manager --no-system-node --alias parentnode
```

```bash
# su rp-owner
redwrap-dnf --redpah /var/redpak/parentnode/childnode manager --alias childnode
```

### Manager Options

```bash
Options:
  --alias=VALUE                 rednode alias name default is node dirname
  --update                      force creation even when node exist
  --no-system-node              Do not create system node: system configuration will be in first parent
  --template=VALUE              Create node config from template [default= /etc/redpak/template.d/default.yaml]
  --template-admin=VALUE        Create node admin config from template [default= /etc/redpak/template.d/admin.yaml]
```

## Launch a node

You can test it executing a simple command as:

```bash
# su rp-owner
redwrap --redpath /var/redpak/parentnode ls
```

Or enter the node with:

```bash
# su rp-owner
redwrap --redpath /var/redpak/parentnode bash
```

## Set up repo into the node

First, it is needed to install the repo file in `/var/redpak/parentnode/etc/yum.repos.d`:

A simple way to test, is to add your system repo file

```bash
# su rp-owner
mkdir -p /var/redpak/parentnode/etc/yum.repos.d
cp /etc/yum.repos.d/redpesk*.repo /var/redpak/parentnode/etc/yum.repos.d
```

## Install a package with redwrap-dnf

Remember that the parent node is depending on "system" node and so depends on system packages,
thus it is forbidden to install a system package inside the node.

So, either you uninstall a package from system and install it inside the node or you
install a package not present in the system.

Below the example with `strace`:

```bash
# su rp-owner
redwrap-dnf --redpath /var/redpak/parentnode/ install strace


# logs
Append db from /var/redpak/parentnode/
Append db from /var/redpak/parentnode/..
Updating repositories metadata and load them:
[0/0] redpesksamples                                  100% | 613.8 KiB/s |   4.3 KiB |  00m00s
[0/0] RedPesk $VERSION - x86_64 - Release             100% |   1.4 MiB/s |   8.1 MiB |  00m06s
Waiting until sack is filled...
Sack is filled.

Package                        Arch     Version                       Repository          Size
Installing:
 strace                        x86_64   5.1-1.el8                     redpesk          1.0 MiB

Transaction Summary:
 Installing:        1 packages

Is this ok [y/N]: y
Downloading Packages:
[1/1] strace-0:5.1-1.el8.x86_64                       100% |   4.6 MiB/s |   1.0 MiB |  00m00s
----------------------------------------------------------------------------------------------
[1/1] Total                                           100% |   4.5 MiB/s |   1.0 MiB |  00m00s

Running transaction:
[1/2] Verify package files                            100% | 100.0   B/s |   1.0   B |  00m00s
[2/3] Prepare transaction                             100% |   1.0 KiB/s |   1.0   B |  00m00s
[3/3] Installing strace-0:5.1-1.el8.x86_64            100% |  22.7 MiB/s |   2.2 MiB |  00m00s
----------------------------------------------------------------------------------------------
[3/3] Total
```

## Excute application with redwrap: strace

You can now launch your application with `redwrap`.

```bash
# su rp-owner
redwrap --redpath /var/redpak/parentnode -- strace ls
```

## List installed applications inside a node

To list all application inside a node and check strace is well installed,
run:

```bash
# su rp-owner
redwrap --redpath /var/redpak/parentnode -- rpm -qa

# logs
strace-5.1-1.el8.x86_64
```

## Remove application with redwrap-dnf

To remove application use redwrap-dnf remove

```bash
# su rp-owner
redwrap-dnf --redpath /var/redpak/parentnode remove strace

# logs
Package                        Arch     Version                       Repository          Size
Removing:
 strace                        x86_64   5.1-1.el8                     @System          2.2 MiB

Transaction Summary:
 Removing:          1 packages

Is this ok [y/N]: y
Downloading Packages:
----------------------------------------------------------------------------------------------
[0/0] Total                                           100% |   0.0   B/s |   0.0   B |  00m00s

Running transaction:
[1/2] Prepare transaction                             100% | 500.0   B/s |   1.0   B |  00m00s
[2/2] Erasing strace-0:5.1-1.el8.x86_64               100% | 600.0   B/s |  15.0   B |  00m00s
----------------------------------------------------------------------------------------------
[2/2] Total
```

## Redconf

`redconf` is command line tool to visualise config and nodes.

### Commands:

* [tree](#tree): visualise node tree
* [config](#config): display/check config of a node
* [mergeconfig](#mergeconfig): display/check the merge config of a node

### Tree

The *tree* command allow to visualise the node tree from a redroot. The redroot is the root of a node family.

```bash
# su rp-owner
redconf tree --redroot /var/parentnode/

# logs
/var
  └── redpak  (system)
      └── parentnode  (parent)
          ├── child2  (child2)
          ├── child1  (child1)
          │   ├── grandchild2  (grandchild2)
          │   └── grandchild1  (grandchild1)
          └── child3  (child3)
```

### Config

The `config` command allow to display/check config yaml file of a node by specifying the redpath of the node.

```bash
redconf config --redpath /var/redpak/parentnode/child1

# partial logs
---
headers=> alias=child1 name=XXXXX info=XXX
config:
	cachedir: (null)

...

exports:
  - [0] mode:  Restricted
         mount: /nodes/_private
         path:  $NODE_PATH/private
  - [1] mode:  Restricted
         mount: /var/lib/rpm
         path:  $NODE_PATH/var/lib/rpm
  ...
---

---
  state=Enable
  realpath=/var/redpak/parentnode/child1
  timestamp=1652693403043
  info=created by XXXX
---
```

The option `--yaml` allows to return the yaml file.

```bash
redconf --yaml config --redpath /var/redpak/parentnode/child1
```


### Merge config

The `mergeconfig` config command allows to display/check the merged config of a node. That is to say, the configuration of the current node and the concatenation of its parents. Remember that a node inherits from its parents contraints and so some warnings can appeared into it.

* `--expand` option expands the environment variable of the configuration file.
* `--yaml` option return the yaml merged config file.


```bash
# su rp-owner
redconf mergeconfig --expand -r /var/redpak/parentnode/child1

# partial logs
----CONFIG 2693
headers:
  alias: child1
  name: XXXX
  info: Node created by XXXX the XXXX
exports:
- mode: Restricted
  mount: /lib64
  path: /usr/lib64
  info: /var/redpak/

...

- mode: Restricted
  mount: /nodes/parentnode/usr
  path: /var/redpak/parentnode//usr
  info: /var/redpak/parentnode/

...

- mode: Public
  mount: /nodes/child1/var
  path: /var/redpak/parentnode/child1/var
  info: /var/redpak/parentnode/child1
environ:
- mode: Default
  key: PS1
  value: Rednode(child1)>
  info: /var/redpak/

...

- mode: Default
  key: XDG_RUNTIME_DIR
  value: /run/user/
  info: /var/redpak/parentnode/

...

- mode: Default
  key: XDG_RUNTIME_DIR
  value: /run/user/
  info: /var/redpak/parentnode/child1
  warn: value is overload by /var/redpak/parentnode
config:
...
  share_user: Unset
...
----
```

See the warn example that explicity said that this value is overload by a parent node:

```bash
- mode: Default
  key: XDG_RUNTIME_DIR
  value: /run/user/
  info: /var/redpak/parentnode/child1
  warn: value is overload by /var/redpak/parentnode
```