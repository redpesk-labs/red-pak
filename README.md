<!-- vim-markdown-toc GFM -->

* [red-pak: an Ultra Light Weight Container for embedded applications](#red-pak-an-ultra-light-weight-container-for-embedded-applications)
    * [Important note](#important-note)
    * [Videos](#videos)
    * [General presentation](#general-presentation)
    * [Main differences with other existing containers](#main-differences-with-other-existing-containers)
        * [Docker, LXC, ...](#docker-lxc-)
        * [Snap, Flatpak, Appimage, ...](#snap-flatpak-appimage-)
        * [Red-pak](#red-pak)
* [The red-family](#the-red-family)
    * [Tools](#tools)
    * [Concepts](#concepts)
    * [Underlying technologies used and their modifications](#underlying-technologies-used-and-their-modifications)
* [Performance](#performance)
* [Quick Start](#quick-start)
* [Configuration](#configuration)
* [Cgroups](#cgroups)
    * [Cgroup Issues](#cgroup-issues)

<!-- vim-markdown-toc -->

# red-pak: an Ultra Light Weight Container for embedded applications

## Important note
Red-pak is Work-In-progress.

Binary packages can be found https://download.redpesk.bzh/redpesk-lts/ and be used on redpesk os.

Please have a look to https://docs.redpesk.bzh/docs/en/master/getting_started/quickstart/03-boot-images.html to boot a redpesk image.

Then you can install redpak with
```bash
dnf install red-pak
```

## Videos
   - Introduction to concepts and architecture: https://vimeo.com/412131602
   - Quick demo on how to install the AGL framework and helloworld demo: https://vimeo.com/412135018

## General presentation
Red-pak targets embedded and critical infrastructures:
   - it maximizes resource sharing (no rootfs/sharedlib duplication)
   - it is designed to be auditable. While each individual node is independent, the global coherency is provided by `dnf`/`rpm` and `libsolv` at the core OS level). The red-pak coherency can be statically proven at the CI level before pushing the image to the target.
   - it simplifies container inspection (a node is an atomic subset of a rootfs)
   - it uses standard management tools (`dnf`+`rpm`)
   - built with long term support and cybersecurity in mind.

## Main differences with other existing containers
### Docker, LXC, ...
   - mostly IT/Datacenter-oriented containers
   - They duplicate the rootfs
   - They generally run or at least start as privileged users
   - They share as little resources as possible with the host (to provide portability)
   - They have a lot of tools to handle Devops operations (backup, migration, elasticity, ..)
### Snap, Flatpak, Appimage, ...
   - Desktop driven containers
   - Except for Flatpak they also duplicate the rootfs
   - Like red-pak they run without privileges
   - Because they target ease of application installation, they cannot maximize resource sharing with the host core OS.
   - They have less tools (mostly application stores) and try to appear on your system as standard applications.
   - The main goal is to address Linux fragmentation, allowing the same binary to run on every Linux flavour/version.
### Red-pak
   - A management-driven system for the embedded world
   - An embedded and generally every critical system is fully managed. Every configuration and combination should be tested before getting to production. The fact your application SHOULD work with ALMOST every configuration without having to be tested is not good enough.
   - Embedded target are generally limited in resources (RAM,CPU,DISK,POWER,...). Duplicating libraries/tools on disk/memory is not an option.
   - Embedded systems need to be auditable/certifiable. A black box container model is not acceptable.

Everyone understands that installing a software component on millions of cars, on a submarine or in a train is very different from installing a new application on a desktop or a phone.

red-pak targets embedded devices used within critical infrastructure (automotive, boat, submarine, train, civil infrastructure, medical, ...). red-pak does not use black box containers, on the contrary it enforces a white box model where the global coherency of the system can be proven. While each node of a red-pak family owns an atomic subset of the full rootfs tree, the global coherency of each node is statically verified. Installation/updates can be proven before installation on the embedded target by an adequate CI/Build-system such as Redpesk (http://docs.redpesk.bzh).

# The red-family

## Tools

- `red-wrap`: creates execution namespaces
- `redwrap-dnf`: handles the nodes lifecycle (target development time)
        + creation
        + adding packages repositories
        + install/update packages
        ...


## Concepts

- **Redpath**: represents a family of nodes that are imported in a namespace at application launch time. Every action on red-pak is done by providing the path to the terminal node of its family tree.

- **RedNode**: the atomic component of red-pak. A node is an independent subset of a full tree rootfs. Each node owns a private RPMdb and repository lists. A node can be installed or updated independently. Each Rednode owns the following items:
   + Alias: should be unique within your Redpath, but may not be globally (i.e. two versions of QT may have the same alias)
   <!-- + XXX: moved Redpath def above as it's referenced here -->
   + Name: it should be unique. It is used for debugging purposes (this is very useful when multiple nodes share the same alias)
   + Config: holds the environment, path, flag, ... for the corresponding node
   + Exports: a list a directories/files a node imports/exports.

- **Redmain**: the core OS and default configuration. Mostly exposes default values for nodes at run time.

- **Exports**: the modes below allow to control the visibility part or all of the tree family. Attributes are:
   + Public: the share point is accessible read/write from every children.
   + Restricted: same as for Public but visibility is read-only instead.
   + Private: Visible only from an application running in the same node (not from the node children).
   + Anonymous: not visible outside the current name space (even two applications running in the node have separated namespaces)
      <!-- + XXX: mixing node/namespace terminology. -->
   + ExecFd: resulting file will contain the result of a shell command execution (i.e: one line of /etc/passwd)
   + Symlink: symbolic link
> **_NOTE:_** a mount may indifferently import directories or files.

- **Share/unshare**:
   + List of tag to expose: network, pid, users, ...
      <!-- + XXX: need a def of a 'tag' -->

- **Environment**: controls the environment of the spawned process. Possible
  values:
   <!-- + XXX: need monospace formatting of types as those are code -->
   + Static: export a fixed key/value pair in the execution environment
   + Expanded: interpolate `$VALUE` before exporting a key/value pair (i.e. Node_Alias, UID, ...)
   + ExecFd: export the output of a shell command execution
   + Remove: remove a variable from the environment before entering the application namespace

## Underlying technologies used and their modifications
- `libdnf`:
    - Use libdnf5
- `rpm`:
    - No change to `librpm.so`, only need `librpm-devel` to compile
- `bubblewrap`:
    - No change to `bwrap`
    - As today `red-wrap` execs the `bwrap` command (in the future this could change in favor of a direct call)

# Performance

More tests need to be done, with many different conditions/configuration (target, versions, ...)

- On my home laptop I used the following sample scenario with:
   - 3 nodes (redpesk-dev/agl-plateform/agl-demos-> demo->applications)
   - 25 mounts (resources imports)

Namespace Start/Stop time is around 50-60ms (depends on global system load)

- To check on your own system you can use the following code:

```
    TIMEMS=`echo "- $(($(date +%s%N)/1000000))  + "  && `redwrap` --rpath=/var/redpesk/agl-redpesk9/agl-demo -- cat </dev/null ; echo $(($(date +%s%N)/1000000))`; echo $(($TIME))
```

Do not forget to retrieve the time measurement cost:

```
    OVERLOAD=`echo "- $(($(date +%s%N)/1000000))  + " ; echo $(($(date +%s%N)/1000000))`; echo $(($TIME))
```

# Quick Start

0. Create a non privileged pool to host your Rednodes

    + `sudo mkdir /var/redpesk`
    + `sudo chown $USER /var/redpesk`

1. Create your 1st node

    + `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9 manager --create --alias=agl-core`

1. Add a repository of rpm packages to your node

    For now it misses a command, so you need to create a .repo file into /var/redpesk/agl-redpesk9/etc/yum.repos.d

```bash
mkdir -p /var/redpesk/agl-redpesk9/etc/yum.repos.d

cat << EOF > /etc/yum.repos.d/repo.repo
[repo]
name=repo
baseurl='https://community-app.redpesk.bzh/kbuild/repos/repo/latest/$basearch'
enabled=1
repo_gpgcheck=0
type=rpm
gpgcheck=0
skip_if_unavailable=False
EOF
```


> **_NOTE:_** My sample URL is not public

<!-- - XXX: need explanations/details on what should be fixed -->

> **_NOTE:_** For testing purpose you simply can copy from your system, e.g.:

```bash
cp /etc/yum.repos.d/redpesk.repo /var/redpesk/agl-redpesk9/etc/yum.repos.d
```

1. Install a package in your node

    + `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9 repoquery *binder*`
    + `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9 install afb-binder`

1. In the node: Check installation succeeded and check application can start

    + `redwrap --redpath=/var/redpesk/agl-redpesk9 -- rpm -qa`
    + `redwrap --redpath=/var/redpesk/agl-redpesk9 -- afb-binder --version`

> **_NOTE:_** a this level we only have one node. If you want to compare with other existing containers technologies, you should think of the red-pak container as a Russian doll. Your container is not one node, but the imbrication of all nodes contained in your Redpath. i.e: node:core OS/node:agl-plateform/node:boat-middleware/node:my-projet->my-navigation-application

1. Enter 'bash' in your container

    <!-- + XXX: explain the `--force` + provide simpler binaries to be run instead of
      the AGL ones -->
    +  `redwrap --redpath=/var/redpesk/agl-redpesk9 --force bash `
    ```
        > ls /
        > ls /nodes/agl-core/usr/bin/
        > afb-binder --version
        > ldd /nodes/agl-core/usr/bin/afb-binder
        > rpm -qa
    ```

1. Add a new leaf to our tree

    + `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo manager --create --alias=agl-demo`

1. Add a repository to your agl-demo node

1. Install the helloworld demo

    +  `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo repoquery *agl*`
    +  `redwrap-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo install agl-service-helloworld.x86_64`

1. Enter your new node

    + `redwrap --redpath=/var/redpesk/agl-redpesk9/agl-demo --force bash`
    + `rpm -qa`
    + `afb-binder --ldpaths=/nodes/agl-demo --workdir=. --roothttp=/nodes/agl-demo/var/local/lib/afm/applications/helloworld-binding/htdocs/ --verbose`

Outside the node:

    + `browser http://10.20.101.105/1234`
    + ` curl -H "Content-Type: application/json" http://localhost:1234/api/helloworld/ping`

1. delete a node (they are atomic, a simple 'rm' is enough)

    + rm -rf /var/redpesk/agl-redpesk9/agl-demo

# Configuration

- main config is at `$ROOT_PATH/etc/red-pak/main.yaml`
- node config is at `$NODE_PATH/etc/redpack.yaml`

Notes on configuration:
   - All sections but 'config/tags' accumulate.
   - Every export path, mounting point or environment variable defined within each node of a given redpath are visible at runtime from the application namespace.
   - Config/tags are unique and merged at run time. Tags defined within `redmain.yaml` are used as default values. Then within a redpath, the oldest ancestor has priority. As the result, if a parent enforces a constraint, children cannot overload the selection (i.e rpm signature require, unshare network, ...)

   <!-- XXX: need explanation on what is in parenthesis -->

# Cgroups

Cgroups allow to distribute system resources along a hierarchy.

Redpak can manage cgroups v2 for its nodes.

Below an example of cgroup configuration for a node.

```yaml
config:
  share_all: Unset
  share_user: Unset
  ...
  cgroups:
          cpuset:
                cpus: 0-2
                mems: 0
                cpus_partition: member
          mem:
                  min: 10M
                  low: 10M
                  high: 521M
                  max: 512M
                  oom_group: 10
                  swap_high: 1024M
                  swap_max: 512M
          cpu:
                  max: 1000 1000
                  weight: 200
                  weight_nice: 10
          io:
                weight: 200
                max: 8:0 rbps=2097152 wiops=120
          pids:
                max: 50

```

The configuration respects the definition of https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html for cgroup v2.

## Cgroup Issues

### V2 Issue

Redpak handles cgroups only in pure v2 version. If you are in cgroup v1 or hybrid, you need to reboot by appending to the command line:

```bash
systemd.unified_cgroup_hierarchy=1
```

After reboot, you can check that `/sys/fs/cgroup' is in v2 with:

```bash
mount -l | grep /sys/fs/cgroup
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```

### User issue

If you don't have right to create a sub cgroup into your current parent cgroup, it may be due to the fact that your not in a user cgroup session.

You can try to start one with:

```bash
systemd-run --user --pty -p "Delegate=yes"  bash
```

### Controller Issue

If you don't manage to write into some controllers, it may be due to a issue in delegation. For that, you need to verify from /sys/fs/cgroup to your parent cgroup (`cat /proc/self/cgroup`), the available controllers `cat cgroup.controllers` and the delegated controllers to their children `cgroup.subtree_control`.

A temporary way to test it, is to append the missing ones at each level, for example:

```bash
echo "+cpuset +cpu +memory +io +pids" > cgroup.subtree_control
```

After you can check that in clild, you have them in controlles

```bash
# in child
cat cgroup.contollers
```

### Issue writing into controllers

All of the elements of redpak config file respects kernel definitions, please have a look at https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html.