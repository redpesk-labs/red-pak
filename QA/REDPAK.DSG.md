# Software design of redpesk-labs/red-pak

.VERSION: 2.2.3

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as REDPAK.

## Overview

For a short introduction to *REDPAK*, see @REDPAK.OVE.

For high level requirements of *REDPAK*, see  @REDPAK.HRQ.

*REDPAK* is a set of programs and plugins providing facilities
for setting up isolated execution environments, the *REDNODE*s,
and for running programs in these environments.

The description of the isolated environments, the *REDNODES* is
done using configuration files in the filesystem.

A *REDNODE* is intended to set an isolated environment for running programs.
The setting of the running environment and the activation of the initial
program is achieved by the program *REDWRAP*.

*REDWRAP* uses the *out of the shelves* software component **bubblewrap**
for setting up namespaces, capabilities and mounting items of the
isolated filesystem.

*REDPAK* also provides components allowing system's package manager
(the couple DNF, RPM) to install items in *REDNODE* and allowing
to safely run system's package manager inside *REDNODE*s.

## REDPAK delivery

### Interfaces of REDPAK delivery

*REDPAK* packages's components leverage the below components:

- **cyaml** is a library for reading YAML files. It is used for reading
  configuration files of *REDNODE*s.

- **bubblewrap** is a program that prepares the isolated environment
  and runs an initial programm in it. It is used as the last stage
  of *REDWRAP* after setting of CGROUPS and usernamespace.

- **linux** is used for setting CGROUPS, namespaces and capabilities
  environments of the launched processes.

*REDPAK* package provides components used by:

- **rpm** is a program installing components delivered in RPM files.
  It uses the redpak plugin for running installation scripts
  in isolated environement.

- **red-microdnf** is a package manager. It uses *REDPAK* extension
  to accept option for installing components in *REDNODE*s.

The below figure shows interfaces of *REDPAK*:

![Figure: redpak environment](assets/REDPAK-fig-interfaces.svg)

### Content of REDPAK delivery

The delivery is made of three subsets:

- The core: **redpak-core**. It contains the tools
  *REDCONF* and *REDWRAP* and their core libraries.

- The package manager adaptor: **redpak-dnf**. It contains
  plugins for the package manager system and for the program
  *redwrap-dnf*.

- The python adaptor: **python3-redconf**. It contains a python
  interface to *REDCONF* library.

The below figure shows the delivery, its components and its links
to interfaces:

![Figure: redpak delivery](assets/REDPAK-fig-redpak.svg)


## REDNODEs

### Nature of REDNODEs

As explained in @REDPAK.OVE, a *REDNODE* is a lightweight container.
As such it is made of several parts:

- a configuration: that defines how the isolated environment of *REDNODE*
  is setup.

- specific data: either data or programs and libraries that can be
  specific to the *REDNODE*.

- programs running in the *REDNODE*. This is the dynamic part.

Parts of the *REDNODE* are static and belong into the the filesystem
as files and directories. Here files can be data, programs or special
files like named sockets or pipes.

An other part is dynamic, it resides in memory as processes and their data
and as kernel objects managing namespaces, cgroups, capabilities, ...


### REDNODE's configuration is in filesystem

The isolated environments *REDNODE*s are described by configurations
files of the filesystem.

**MOTIVATION**:

Having rednode configuration in an extra database adds a database
component that (1) is a failure point (2) is a contention point
(3) complicates administration and debugging.

### Structure of a REDNODE

A *REDNODE* is a directory containing the following items:

- a *REDNODE* status file named `.rednode.yaml` tagging the directory as a *REDNODE*
- a configuration directory named `etc`
- a *REDNODE* configuration file for normal operations named `etc/redpak.yaml`
- *optionally*, a *REDNODE* configuration file for administrative operations named `etc/redpak-admin.yaml`

The below figure summarizes that structure, **BASE** being the *root* directory
of a *REDNODE*.

```
BASE
  ├── .rednode.yaml
  └── etc
        ├── redpak.yaml
        └── redpak-admin.yaml
```

***NOTE*** On previous versions of *REDPAK* the files in the subdirectory `etc`
were named `redpack.yaml` and  `redpack-admin.yaml`. So *REDPAK* checks if such
file exist for using it when current names do not exist.

### Administrative configuration

For administrative purposes, *REDNODE* configuration might optionally
define specific "administrative" configuration.

When administrative request is required, the administrive configuration
is used if it exists.

### Other content of a rednode

The **BASE** directory of a *REDNODE* is indexed by *$NODE_PATH* or
*$LEAF_PATH* in configuration files.

It contains contains at least the configuration of the *REDNODE*.

It should also contains content specific to the *REDNODE* in a directory
mounted in the *REDNODE*. For example, default configuration files are using
the directory *$NODE_PATH/private*.

## Organisation of REDNODEs

Usage analysis shows the need to be able to group *REDNODE*s.

### Layered and merged configuration

Setting up the configuration of an isolated environment, a *REDNODE*,
is an engineering process that can be achieved using different methods
comprising the below ones:

1. Start with an empty *REDNODE* and try to execute the targeted
   execution processes in it. When a part of the system is missing,
   check it if it' i's really needed. If so, add it to the *REDNODE*
   configuration and then iterate.

2. After analyzing the real needs of the executed processes, set
   the configuration and magically it works.

3. Starting from a full featured *REDNODE*, drop any features that
   design requirements express to be dropped. Processes executed
   in the *REDNODE* are restricted as expected.

4. Knowing a catalog of existing *REDNODE*, choose the
   one that best fits the requirements and use its setting as initial
   *REDNODE* setting.

The fourth solution is one of the most convenient, but to make it really
usable, at least two features should be added altogether:

- It should be possible to tune the template setting used initially to improve it.

- It should be possible to change the template setting used initially 
  in such way that it applies to all *REDNODE*s based on it (for example
  for refinement or bug fix).

These concerns lead to concepts of layered and merge configurations:
instead of copying the initial template setttings, build on it and when
it changes, its children benefit of the changes.

### Grouping REDNODEs

Isolating processes is very convenient but processes should sometime
share data or communicate together. It is sometime interesting to
define groups of processes working togetherbut separated and isolated
each another.

The sames concepts of layered and merged configuration can also serve the
purpose.

In that case, the group shares a common configuration that isolates
the group from the rest of the system but shares a same common area
for file or memory data and for services. Each separate process of the
group refines its configuration to isolate itself from the other processes
of the group.

### REDNODE layered hierarchy is in filesystem

.REQUIREMENT REDPAK.DSG-R-RED-LAY-HIE-FIL
Filesystems implement directories as a tree structure. That tree
structure is used for implementing *REDNODE* layered structure.

**MOTIVATION**:

Structure of filesystem is hierarchical. Relying on it
for holding hierachy of nodes is natural and allows
administative actions to be performed with standard
CLI tools.


### Structure of REDNODE layered hierarchy

In file systems, the direct parent directory of a directory **A**
is the directory that directly contains the directory **A**.

If the direct parent directory of a *REDNODE*, named **X**, is also
a *REDNODE*, named **P**, then the *REDNODE* **P** is the _direct_ parent
*REDNODE* of **X**.

A *REDNODE* named **ANCESTOR** is a parent *REDNODE* of **NODE** if **ANCESTOR**
is a _direct_ parent of **NODE** or if the *REDNODE* **PARENT** is the
_direct_ parent *REDNODE* of **NODE** and that **ANCESTOR** is a
parent *REDNODE* of **PARENT**.

It is important to note that:

- only direct directory are taken into account, meaning any directory
  not containing a *REDNODE* breaks the relation.

- the definition of parent *REDNODE* is recursive.


For any *REDNODE*, two cases are possible:

1. the *REDNODE* has no direct parent *REDNODE*, it is said to be a root *REDNODE*.

2. the *REDNODE* has one direct parent *REDNODE*, it is said to be a child *REDNODE*.

A child *REDNODE* can also be a parent *REDNODE*.


## Configuration of REDNODEs


### REDNODE's configuration files are YAML

.REQUIREMENT REDPAK.DSG-R-RED-CON-FIL-ARE-YAM

The configuration files of *REDNODE*s are YAML files.

**MOTIVATION**:

The YAML format has advantages (1) it is a text file that can
be viewed and edited with basic commands (2) it is human readable
(3) it accept comments.


### Final configuration of REDNODE

The final configuration of a root *REDNODE* is the configuration
written in its configuration file.

The final configuration of a child *REDNODE* is the merge of the
the configuration written in its configuration file and the final
configuration of its direct parent *REDNODE*.

In other words, the final configuration of a *REDNODE* is
the merge of its configuration and the configuration of all
its parent *REDNODE*s.


### Final administrative configuration of REDNODEs

*REDNODE*s can have specific administrative configuration.
Administrative configuration is used by default when installing
packages in *REDNODE*s.

Administrative configuration is set in a specific configuration
file in *REDNODE*s: `$NODE_PATH/etc/redpak-admin.yaml`.
Settings of this file is merged to the final configuration using
specific rules.

## Management CGROUP for red nodes

When CGROUP configuration is given, *REDWRAP* tries to apply the required
settings for the *REDNODE* in the CGROUP hirarchy. It can only be done if
*REDWRAP* has enough right to achieve that job.

This job must take care of the [no internal process constraint][1].

For being able to escape from the root cgroup (the cgroup
holding the *REDWRAP* or, alternatively, the cgrouo path set for
configuration key **config.cgrouproot**), that "root" cgroup should
not contain any PIDS. It implies that processes of that root group
if existing should be moved in a special child cgroup intended to
hold processes. *REDWRAP* firstly ensure it. The special cgroup is named
`redpak-pids-leaf`.

Then, for each *REDNODE* of the hierarchy, starting with the root ancestor
and finishing with the target *REDNODE*, two cgroups are created: one for
setting controlers with required configuration, designated below as *CONTROL*,
and one cgroup, child of *CONTROL*, for holding pids, designated below as *PID*.

Cgroups *PID* are all named `redpak-pids-leaf`. They are only used to
hold PIDs of processes.

Cgroups *CONTROL*, are named after *alias* name of *REDNODE*s and nature of
the configuration: normal or administrative. Cgroups for normal configuration
are named *ALIAS*. Cgroups for administrative configuration are named
*ALIAS.admin*. These cgroups are used for setting the configuration of
the controllers as required by configuration files.

*REDWRAP* also try to activates the expected controllers by applying
recursively requests on controllers (see [controlling-controllers][2])

The figure below shows cgroup hierarchy for 3 nodes in administrative
configuration:

![Figure: cgroup hierarchy](assets/REDPAK-fig-cgroups.svg)

In the above figure, because the *REDNODE* *node-b* has no admin configuration,
is not represented by a CGROUP admin. But it is distinct from normal *node-b*
*REDNODE* instances that are in CGROUP `<ROOT>/node-a/node-b`, not in
`<ROOT>/node-a.admin/node-b`.

## Merges rules

When hierarchy of *REDNODE*s is used, the merging rules defined below
are used to compute the settings to apply to *REDNODE*s based on the content
of the configuration files.

The rules are grouped in categories:

- Merge of path like values: for `config.path` and `config.ldpath` items
- Merge of environment variables: for `environ[*]` items
- Merge of capabilities: for `config.capabilities[*]` items
- Merge of exports: for `exports[*]` items
- Merge of Sharings: for `config.share_(all|user|cgroup|net|pid|ipc|time)` items
- Merge of cgroups: for `config.cgroups.*` items
- Merge of other configuration items

### Merge of path like values

This chapter describe the merge of `config.path` and `config.ldpath` items.

The special environment variables **PATH** and **LD_LIBRARY_PATH**
are merged.

Merge rules:

1. items of child node appear before items of its parents
2. items of admin appear before item of normal
3. duplication is avoided


### Merge of environment variables

This chapter describe the merge of  `environ[*]` items.

Environment variables can be redefined.

Merge rules:

1. a normal child node can redefine an environment variable of a normal parent
2. an admin child node can redefine an environment variable of a parent
3. an admin value take precedence on normal

Merge matrix:

| PARENT | NORMAL | ADMIN | EFFECT     |
|--------|--------|-------|------------|
|   -    |   N    |   -   |   N
|   P    |   -    |   -   |   P
|   NP   |   N    |   -   |   N - WARN
|   AP   |   N    |   -   |   AP
|   *    |   *    |   A   |   A - WARN

Legend:

- -: unset,
- *: any value,
- N: normal definition,
- A: admin definition
- P: parent definition,
- NP: normal prent definition,
- AP: admin parent definition


### Merge of capabilities

This chapter describe the merge of `config.capabilities[*]` items.

Merge rules:

1. a child node can not enable a capability disabled by a parent
2. an admin config can enable a capability disabled by a normal node
3. when admin is required, a child normal node can't change admin parent settings

Merge matrix:

| PARENT | NORMAL | ADMIN | EFFECT       |
|--------|--------|-------|--------------|
|   -    |   -    |   -   |   -          |
|   -    |   0    |   -   |   0N         |
|   -    |   1    |   -   |   1N         |
|   -    |   *    |   0   |   0A         |
|   -    |   *    |   1   |   1A         |
|--------|--------|-------|--------------|
|   0N   |   -    |   -   |   0N         |
|   0N   |   0    |   -   |   0N         |
|   0N   |   1    |   -   |   ERROR      |
|   0N   |   *    |   0   |   0A         |
|   0N   |   *    |   1   |   1A         |
|--------|--------|-------|--------------|
|   1N   |   -    |   -   |   1N         |
|   1N   |   0    |   -   |   0N - WARN  |
|   1N   |   1    |   -   |   1N - WARN  |
|   1N   |   *    |   0   |   0A         |
|   1N   |   *    |   1   |   1A         |
|--------|--------|-------|--------------|
|   0A   |   *    |   -   |   0A         |
|   0A   |   *    |   0   |   0A - WARN  |
|   0A   |   *    |   1   |   ERROR      |
|--------|--------|-------|--------------|
|   1A   |   *    |   -   |   1A         |
|   1A   |   *    |   0   |   0A - WARN  |
|   1A   |   *    |   1   |   1A - WARN  |

Legend:

- -: unset,
- *: any value,
- 0N: normal disabled,
- 1N: normal enabled,
- 0A: admin disabled,
- 1A: admin enabled


### Merge of exports

This chapter describe the merge of exports[*]` items.

Merge rules:

1. a normal child node can redefine an export of a normal parent
2. an admin child node can redefine an export of a parent

Merge matrix:

| PARENT | NORMAL | ADMIN | EFFECT     |
|--------|--------|-------|------------|
|   -    |   N    |   -   |   N        |
|   P    |   -    |   -   |   P        |
|   NP   |   N    |   -   |   N - WARN |
|   AP   |   N    |   -   |   AP       |
|   *    |   *    |   A   |   A - WARN |

Legend:

- -: unset,
- *: any value,
- N: normal definition,
- A: admin definition
- P: parent definition,
- NP: normal parent definition,
- AP: admin parent definition


### Merge of sharings

This chapter describe the merge of `config.share_(all|user|cgroup|net|pid|ipc|time)` items.

Merge rules:

1. a child node can not enable a sharing disabled by a parent
2. an admin config can enable a sharing disabled by a normal node
3. when admin is required, a child normal node can't change admin parent settings

Merge matrix:

| PARENT | NORMAL | ADMIN | EFFECT  |
|--------|--------|-------|---------|
|   -    |   -    |   -   |   -     |
|   -    |   0    |   -   |   0N    |
|   -    |   1    |   -   |   1N    |
|   -    |   *    |   0   |   0A    |
|   -    |   *    |   1   |   1A    |
|--------|--------|-------|---------|
|   0N   |   -    |   -   |   0N    |
|   0N   |   0    |   -   |   0N    |
|   0N   |   1    |   -   |   ERROR |
|   0N   |   *    |   0   |   0A    |
|   0N   |   *    |   1   |   1A    |
|--------|--------|-------|---------|
|   1N   |   -    |   -   |   1N    |
|   1N   |   0    |   -   |   0N    |
|   1N   |   1    |   -   |   1N    |
|   1N   |   *    |   0   |   0A    |
|   1N   |   *    |   1   |   1A    |
|--------|--------|-------|---------|
|   0A   |   *    |   -   |   0A    |
|   0A   |   *    |   0   |   0A    |
|   0A   |   *    |   1   |   ERROR |
|--------|--------|-------|---------|
|   1A   |   *    |   -   |   1A    |
|   1A   |   *    |   0   |   0A    |
|   1A   |   *    |   1   |   1A    |

Legend:

- -: unset,
- *: any value,
- 0N: normal disabled,
- 1N: normal enabled,
- 0A: admin disabled,
- 1A: admin enabled

### Merge of cgroups

This chapter describe the merge of `config.cgroups.*` items.

Implementation of cgroups leverage the hierarchy offered by
linux's cgroups. For *REDNODES*, it means that the merge
is achieved by linux kernel composition of cgroups.

For one node, the merge is meaningful only for the configuration
administrative versus normal using the below rules:

1. normal nodes only apply normal configuration
2. normal nodes apply merge of admin and normal configuration
3. admin configuration override normal configuration



### Merge of other configuration items

This chapter describe the merge of other `config.*` items.

Merge rules:

1. sharings are merged as sharings
2. cgroups are merged as cgroups
3. other values are set by the last item

Merge matrix:

| PARENT | NORMAL | ADMIN | EFFECT |
|--------|--------|-------|--------|
|   P    |   -    |   -   |   P    |
|   -    |   N    |   -   |   N    |
|   NP   |   N    |   -   |   N    |
|   AP   |   N    |   -   |   AP   |
|   *    |   *    |   A   |   A    |

Legend:

- -: unset,
- *: any value,
- N: normal definition,
- A: admin definition
- P: parent definition,
- NP: normal prent definition,
- AP: admin parent definition




[1]: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html#no-internal-process-constraint
[2]: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html#controlling-controllers

