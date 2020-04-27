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

<!-- vim-markdown-toc -->

# red-pak: an Ultra Light Weight Container for embedded applications

## Important note
Red-pak is Work-In-progress. Binary packages should come soon. In the meantime early adopters should work from source.

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

- `red-dnf`: handles the nodes lifecycle (target development time)
        + creation
        + adding packages repositories
        + install/update packages
        ...

- `red-rpm`: provides rpm management on the target. It is also used to install packages coming from an 'Over the Air' update.

- `red-wrap`: creates execution namespaces

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
   Note: a mount may indifferently import directories or files.

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
- `dnf`/`libdnf`: 
    - Unfortunately as of today red-pak requires a small patch on `libdnf` (~100 lines)
    - Uses a custom main CLI to set proper arguments
       <!-- - XXX: need explanation -->
    - Ships a C++/Python module that is used by red-plugins
- `rpm`:
    - No change to `librpm.so`, only need `librpm-devel` to compile
    - Shipping a custom version of the librpm/Python interface
    - Shipping a custom `red-rpm` CLI that understands the additional `--redpath` argument
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
    TIMEMS=`echo "- $(($(date +%s%N)/1000000))  + "  && `red-wrap` --rpath=/var/redpesk/agl-redpesk9/agl-demo -- cat </dev/null ; echo $(($(date +%s%N)/1000000))`; echo $(($TIME))
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

    + `red-dnf --redpath=/var/redpesk/agl-redpesk9 red-manager --create --alias=agl-core`

1. Add a repository of rpm packages to your node

    +  `red-dnf --redpath=/var/redpesk/agl-redpesk9 red-manager --add-repo http://kojihub.lorient.iot/kojifiles/repos/II--RedPesk-9-build/latest/x86_64`

    Note: you may have multiple repositories per node. (should be fixed) My sample URL is not public 
    <!-- - XXX: need explanations/details on what should be fixed -->

1. Install a package in your node

    + `red-dnf --redpath=/var/redpesk/agl-redpesk9 red-search binder`
    + `red-dnf --redpath=/var/redpesk/agl-redpesk9 red-install agl-app-framework-binder`

1. Start an application in your node

    + `red-wrap --redpath=/var/redpesk/agl-redpesk9 afb-daemon --version`

    Note: a this level we only have one node. If you want to compare with other existing containers technologies, you should think of the red-pak container as a Russian doll. Your container is not one node, but the imbrication of all nodes contained in your Redpath. i.e: node:core OS/node:agl-plateform/node:boat-middleware/node:my-projet->my-navigation-application

1. Enter 'bash' in your container

    <!-- + XXX: explain the `--force` + provide simpler binaries to be run instead of
      the AGL ones -->
    +  `red-wrap --redpath=/var/redpesk/agl-redpesk9 --force bash `
    ```
        > ls /
        > ls /nodes/agl-core/usr/bin/
        > afb-daemon --version
        > ldd /nodes/agl-core/usr/bin/afb-daemon
        > rpm -qa
    ```

1. Add a new leaf to our tree

    + `red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-manager --create --alias=agl-demo`

1. Add a repository to your agl-demo node

    +  `red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-manager --add-repo http://kojihub.lorient.iot/kojifiles/repos/IIExtra--RedPesk-9-build/latest/x86_64/`

1. Install the helloworld demo

    +  `red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-search agl`
    +  `red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-install agl-service-helloworld.x86_64`

1. Enter your new node

    + ls /nodes
    + ls /nodes/agl-core/usr/bin/afb-daemon
    + `red-wrap --redpath=/var/redpesk/agl-redpesk9/agl-demo --force bash`
    + `afb-daemon --ldpaths=/nodes/agl-demo --workdir=. --roothttp=/nodes/agl-demo/usr/agl-service-helloworld/htdocs --verbose`
    + `browser http://10.20.101.105/1234`

1. delete a node (they are atomic, a simple 'rm' is enough)

    + rm -rf /var/redpesk/agl-redpesk9/agl-demo

# Configuration

- main config is at `/xxx/etc/red-pak/main.yaml`
- node config is at `$NODE_PATH/etc/redpack.yaml`

Notes on configuration: 
   - All sections but 'config/tags' accumulate. 
   - Every export path, mounting point or environment variable defined within each node of a given redpath are visible at runtime from the application namespace.
   - Config/tags are unique and merged at run time. Tags defined within `redmain.yaml` are used as default values. Then within a redpath, the oldest ancestor has priority. As the result, if a parent enforces a constraint, children cannot overload the selection (i.e rpm signature require, unshare network, ...)
   
   <!-- XXX: need explanation on what is in parenthesis -->
