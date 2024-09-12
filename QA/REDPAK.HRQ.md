# High level requirements of redpesk-labs/red-pak

.VERSION: 2.2.3

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as REDPAK.

## Overview

Here is a short introduction. For a more detailed overview
see the document @REDPAK.OVE.

Within REDPAK, the term *REDNODE* refers to a specific execution
environment implementing security isolations.

The isolation of *REDNODE*s is achieved by leveraging linux's namespaces.

At the same time, *REDNODE*s get security improvements by
leveraging following subparts of linux:

- **cgroup** for controlling resource usage of processes running
  in a *REDNODE*.

- **capabilities** for controlling specific administrative behaviours.

Within REDPAK, the term *REDWRAP* refers to the program setting
up the isolated execution environment, the *REDNODE*, and running
inside it isolated processes.



## Requirements


### REDWRAP setup isolation of processes

.REQUIREMENT

The program *REDWRAP* shall setup the isolated execution environment,
a *REDNODE*, by leveraging linux's namespaces an linux's cgroups.

In that environment, *REDWRAP* executes an initial command (process) and
all its forked subcommands (process).

When every processes of the isolated environment are terminated,
*REDWRAP* terminates isolated execution environment, destroying
any artifact living in volatile area.


### REDWRAP always redefines filesystem's root

.REQUIREMENT

The program *REDWRAP* shall always setup a full filesystem root
by leveraging mount namespace.

It means that mount namespace is always setup in *REDNODE*s.


### REDWRAP mounts selected part of host's filesystem

.REQUIREMENT

Inside its fresh empty root filesystem, the program *REDWRAP*
shall be able to mount parts of host's filesystem.


### REDWRAP mounts volatile area using tmpfs

.REQUIREMENT

Inside its fresh empty root filesystem, the program *REDWRAP*
shall be able to mount volatile area using tmpfs.


### REDWRAP leverages all linux's namespaces

.REQUIREMENT

The program *REDWRAP* shall be able to setup isolation of *REDNODE*s for
any namespace available in linux.

Today, the available linux namespaces are:

| namespace | description                          |
|-----------|--------------------------------------|
| Cgroup    | Cgroup root directory                |
| IPC       | System V IPC, POSIX  message queues  |
| Network   | Network devices, stacks, ports, etc. |
| Mount     | Mount points                         |
| PID       | Process IDs                          |
| Time      | Boot and monotonic clocks            |
| User      | User and group IDs                   |
| UTS       | Hostname and NIS domain name         |

Isolation of a namespace is a achieved by by unsharing that namespace.

Note that *Mount* namespace is always unshared.

### REDWRAP leverages parts of cgroup

.REQUIREMENT

The program *REDWRAP* shall be able to setup for *REDNODE*s
the below items of cgroup (v2):

| cgroup item           | description                    |
|-----------------------|--------------------------------|
| cpuset.cpus           | cpu selection                  |
| cpuset.mems           | memory selection               |
| cpuset.cpus.partition | scheduling partition           |
| cpu.weight            | scheduling weight (0 .. 10000) |
| cpu.max               | maximum bandwith               |
| io.max                | per device max settings        |
| io.weight             | scheduling weight (1 .. 10000) |
| memory.min            | minimal memory size            |
| memory.max            | maximum memory size            |
| memory.high           | throttling trigger limit       |
| memory.swap.max       | swap maximum memory size       |
| memory.swap.high      | swap throttling trigger limit  |

### REDWRAP manages Linux's capabilities

.REQUIREMENT

The program *REDWRAP* shall be able to drop or keep any available
linux's capability.

### REDPAK should be integrated in the package manager DNF

.REQUIREMENT

When the package manager DNF is managing the system, it must include
extensions for managing (instal, remove, update, ...) components in
*REDNODE*s.
