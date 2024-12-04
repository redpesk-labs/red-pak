# Risk Analysis of RED-PAK

.VERSION: 2.3.x

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.git-id($Id$)

## Overview

For a short introduction to *REDPAK* and *REDNODE*s, see @REDPAK.OVE.

*REDPAK* is an efficient tool for setting up light container
isolation.

After preseting the possible threats that could break
correct *REDPAK* behaviour and that should be taken into
account, this document establishes guidelines for *REDPAK*
secure usage.

## Risk analysis

Aspects of *REDNODE*s to be inspected are:

- how data of *REDNODE*s are protected
- how processes of *REDNODE*s are protected
- how processes of *REDNODE*s are constrained to isolation
- how configurations of *REDNODE*s are protected
- how configurations of *REDNODE*s are safe

### How data of REDNODEs are protected

Data of *REDNODE*s can be divided in two parts:

- The data that are dynamic and that remain always
  in memory because mounted by *REDWRAP* in *tmpfs*.
  These data are safe by nature because these data are
  only visible by its *REDNODE* and are discarded
  at its end. So these data can only be changed by
  processes running in the *REDNODE*.

- The data that remain in filesystem because mounted partly
  by *REDWRAP* in *REDNODE*s. These data can be accessed by
  *REDNODE*s that mount it and by other part of the system.
  The protection of these data should be achieved using
  Linux's security features like DAC, MAC, ACL.

### How processes of REDNODEs are protected

Processes of *REDNODE*s may be visible by other processes
that are not in *REDNODE*s. These processes are then
sensitives and should be protected. This should be achieved
using Linux's security features like DAC, MAC, ACL.

### How processes of REDNODEs are constrained to isolation

The processes are normally isolated by the couple *REDWRAP*
plus *BUBBLEWRAP* that are putting them in namespaces.

Processes running in such *REDNODE* are constrained by the
setting of the *REDNODE*. So what they can do depends on
the settings of their configuration files.

Nevertheless, using system call *setns*, processes having
the capability **CAP_SYS_ADMIN** are able to escape from
namespaces *user*, *pid*, *cgroup*, *network*, *IPC*, *time* and *UTS*
and processes having the capabilities **CAP_SYS_ADMIN**
and **CAP_SYS_CHROOT** are able to escape from namespace *mount*.
But this escape implies to access PID information of other processes
it wants join. Being in isolated PID namespace does not protect
if procfs is mounted int he *REDNODE*.

### How configurations of REDNODEs are protected

The configuration of *REDNODE*s are sensitive data because
changing it could allow unexpected sharing having effect
in both directions (from inner or outer of *REDNODE*s).

In the universe of *REDPAK*, configurations are key points
because auditing security brought by a *REDNODE* is auditing
its configuration file.

So once audited, configuration should remain exactly has
audited. Protection of configuration files should be achieved
using Linux's security features like DAC, MAC, ACL.

### How configurations of REDNODEs are safe

As seen above, configuration files are sensitive and must be
protected but the question here is how configuration
could be used to damage the system.

Configuration items are processed by *REDCONF*, *REDWRAP*,
*BUBBLEWRAP* and *Linux kernel*.

*REDCONF* is a tool for manipulating configurations. It can
also create *REDNODE*'s structure. For the former case, it should
have authorisation to write configurations. In any case, that
program is not on the critical path and can be run with no
special capability.

The case of  *REDWRAP* and *BUBBLEWRAP* is different because
for some use case, it should be run with elevated privileges
(capabilities). So it should be resilient to malicious
configurations.

*BUBBLEWRAP* and *Linux kernel* are on the critical path but
they are COTS (components off the shelf) and have good
properties: they are safe on their inputs that it validates.
*BUBBLEWRAP* also has the safe property to stop when it encounters
an error. So it can not damage the system by themself with a
malicious configuration.

*REDWRAP* also stops on error detection and when *BUBBLEWRAP*
stops. Actions taken by *REDWRAP* are safe and its input can
damage it. So the threat on *REDWRAP* is on taking control
through a stack overflow but it can not happen because *REDWRAP*
is safe on that point.

## RED-PAK cyber-security recommendations

1. When not STRICTLY needed, the capability **CAP_SYS_ADMIN**
   should be removed.

2. Even if not having static data, applications of separate
   *REDNODE*s should also be separated using DAC (i.e. using
   separate user) or, preferabily but not exclusive, using
   MAC (i.e. using different Smack label).

3. Configuration files must be protected using DAC (i.e. using
   separate user) or, preferabily but not exclusive, using
   MAC (i.e. using different Smack label).

4. Configuration files should not be mounted in *REDNODE*s.

5. Every time it is possible, *REDWRAP* should be invoked
   in not privileged context.
