# High level requirements of redpesk-labs/red-pak

.VERSION: 2.3.x

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ALL

.DIFFUSION: PUBLIC

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as *REDPAK*.

## Overview

Here is a short introduction. For a more detailed overview
see the document @REDPAK.OVE.

Within REDPAK, the term *REDNODE* refers to a specific execution
environment implementing security isolations.

*REDNODE*s get security improvements by leveraging following
parts of linux:

 - **namespace** for isolating processes from the environment:
 filesystem, network, date, users, IPC.

- **cgroup** for controlling resource usage of processes running
  in a *REDNODE*: CPU usage, memory usage, count of processes.

- **capabilities** for allowing specific administrative behaviours.

Within REDPAK, the term *REDWRAP* refers to the program setting
up the isolated execution environment, the *REDNODE*, and running
inside it isolated processes.



## Requirements


### REDWRAP setup isolation of processes

.REQUIREMENT REDPAK.HRQ-R-RED-SET-ISO-PRO
The program *REDWRAP* shall setup the isolated execution environment,
a *REDNODE*, by leveraging linux's namespaces, linux's cgroups and
linux's capabilities.

In that environment, *REDWRAP* executes an initial process and
all its child processes.

When all processes of the isolated environment are terminated,
*REDWRAP* terminates isolated execution environment, destroying
any artifact living in volatile areas.


### REDWRAP always redefines filesystem's root

.REQUIREMENT REDPAK.HRQ-R-RED-ALW-RED-FIL-ROO
The program *REDWRAP* shall always setup a full filesystem root
by leveraging mount namespace.

It means that mount namespace is always setup in *REDNODE*s.


### REDWRAP mounts selected part of host's filesystem

.REQUIREMENT REDPAK.HRQ-R-RED-MOU-SEL-PAR-HOS-FIL
Inside its fresh empty root filesystem, the program *REDWRAP*
shall be able to mount parts of host's filesystem.


### REDWRAP mounts volatile area using tmpfs

.REQUIREMENT REDPAK.HRQ-R-RED-MOU-VOL-ARE-USI-TMP
Inside its fresh empty root filesystem, the program *REDWRAP*
shall mount volatile area using tmpfs.


### REDWRAP leverages all linux's namespaces

.REQUIREMENT REDPAK.HRQ-R-RED-LEV-ALL-LIN-NAM
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

.REQUIREMENT REDPAK.HRQ-R-RED-LEV-PAR-CGR
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

.REQUIREMENT REDPAK.HRQ-R-RED-MAN-LIN-CAP
The program *REDWRAP* shall be able to drop or keep the
41 linux's capabilities listed below.

Required capabilities:
    *cap_chown*,
    *cap_dac_override*,
    *cap_dac_read_search*,
    *cap_fowner*,
    *cap_fsetid*,
    *cap_kill*,
    *cap_setgid*,
    *cap_setuid*,
    *cap_setpcap*,
    *cap_linux_immutable*,
    *cap_net_bind_service*,
    *cap_net_broadcast*,
    *cap_net_admin*,
    *cap_net_raw*,
    *cap_ipc_lock*,
    *cap_ipc_owner*,
    *cap_sys_module*,
    *cap_sys_rawio*,
    *cap_sys_chroot*,
    *cap_sys_ptrace*,
    *cap_sys_pacct*,
    *cap_sys_admin*,
    *cap_sys_boot*,
    *cap_sys_nice*,
    *cap_sys_resource*,
    *cap_sys_time*,
    *cap_sys_tty_config*,
    *cap_mknod*,
    *cap_lease*,
    *cap_audit_write*,
    *cap_audit_control*,
    *cap_setfcap*,
    *cap_mac_override*,
    *cap_mac_admin*,
    *cap_syslog*,
    *cap_wake_alarm*,
    *cap_block_suspend*,
    *cap_audit_read*,
    *cap_perfmon*,
    *cap_bpf*,
    *cap_checkpoint_restore*
