# Usage

## Presirequites

```bash
dnf install red-pak
```

## Create rednodes

### Install a node (With a system node)

The command below create the parent node inside a system node

```bash
redwrap-dnf --redpah /var/redpak/parentnode manager --alias parentnode
```

```bash
redwrap-dnf --redpah /var/redpak/parentnode/childnode manager --alias childnode
```

### Install a node (Without system ndoe)

```bash
redwrap-dnf --redpah /var/redpak/parentnode manager --no-system-node --alias parentnode
```

```bash
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
redwrap --redpath /var/redpak/parentnode ls
```

Or enter the node with:

```bash
redwrap --redpath /var/redpak/parentnode bash
```

## Install package inside a node

First, it is needed to install the repo