# Using CGROUPS with redpak and systemd

REDPAK has options to setup CGROUP restriction on started
REDNODES. This behaviour duplicates the behaviour of
systemd that already manages CGROUP features.

One of the design principle of how systemd manages CGROUP
is that only one writer can control a CGROUP
(see [Two Key Design Rules][1]).

That **single-writer** behaviour can not be enforced by the
kernel. So designers of systemd took the decision that
the full CGROUP hierarchy is under the control of systemd
except if the opposite is explicitely required.

Using CGROUP features of REDPAK without taking care of what
systemd expect is possible but:

- it violates the **single-writer** principle because
  REDWRAP will write and rearrange features in CGROUP
  where systemd expects to be the only writer

- it implies that you run REDWRAP as root or with some
  capabilies for writing where systemd writes

In order to solve this kind of issues, systemd has
provision for allowing other writers to manage parts
of CGROUP that system will not write after setting it up.
This behaviour, called **delegation**, is described in
that [document][1].


## Delegation in systemd's user land

The 2 below lines are from systemd's service `user@.service`

```
Slice=user-%i.slice
Delegate=pids memory
```

It shows that every user service or application started by
systemd and the children of these services or applications
are in the CGROUP slice `user.slice/user-UID.slice/`.

And that slice is restricted to have only access to the
CGROUP's controllers `pids` and `memory`.

So if the REDNODE only need to setup `pids` and `memory`
it could be possible in principle because the controllers
are available. But in fact it will not work because the
files controlling behaviour of these CGROUP are owned by
root and are write protected.

Before discussing on how systemd allows delegation of the
control of these features, let talk about other CGROUP
controllers that REDPAK manages: cpu, cpuset, io.

Because these controllers are not listed in `user@.service`,
it cannot be delegated to user services. This can be
changed by adding drop-in configurations. For example
for adding the missing controllers to user 1001, it is
needed to create the file
`/etc/systemd/system/user@1001.service.d/delegate.conf`
([see][2]) that contains:

```
[Service]
Delegate=cpu cpuset io
```

But it can be seen that this action requires system privileges
to be achieved.

Without delegation, the CGROUP properties can be controlled
by systemd settings and properties.

In order to run redpak in user mode, you need to turn on the
delegation. This is done by including the below lines in your
service:

```
[Service]
Delegate=yes
```

that delegates all available CGROUP controllers.

Or by listing the expected CGROUP controllers as below:

```
[Service]
Delegate=memory pids
```

In this cases, systemd creates a slice for controlling
the service. That slice can be named explicitely using
`Slice` directive of systemd.

When running REDPAK from command line in the user environment,
if the REDNODE has CGROUP setting, they may be applied or not
depending on rigths that systemd let to CGROUPS files. But
in all cases it enters in conflict with systemd and the
**single-writer** rule.

So the systemd command `systemd-run` should be used. Here
is an example of use:

```
systemd-run --user -p Delegate=yes ...
```

Using the slice is possible because it allows default
controllers, so the command below works as well:

```
systemd-run --user --slice redpak ...
```

In this cases, the user still get the ability to
modify its CGROUP restictions itself. This is intended
for implementing a manager but not for running
a restricted program. However redpak allows to run
REDNODEs not able to change their CGROUP restrictions
if the CGROUP namespace is unshared:

```
config:
    share_cgroup: disabled
```


## Delegation in systemd's system land

Restriction applied by `user@.service` are not existing
in system land. So all CGROUP controllers are available.

But the use of the directive `User` even in system's services
make them user services and then only allow controlllers
*pids* and *memory* as seen above. To circumvent this
restriction either follow what is explain above or use
the redpak feature of changing the user.

Since REDPAK version 2.4.2, it is possible to start
a REDNODE from root that will run as an other user.
It opens the possiblity to use all CGROUPS from
system land but in user land. In such case, services
could be change to not set `User=...` while still
setting dependency to `user@%i.service`. That way,
user services are started and all CGROUP controllers are
available.

When using this option, items of CGROUP hierarchy are
owned by root, not by the end user. This is good because
it avoid the REDNODE to change its settings.

## REDNODEs, CGROUPS, and russian dolls

For CGROUP management of REDNODE, the concept
of russian dolls applies, the hierarchy of rednodes
is applied by fitting children in their parent.
This behaviour automatically applies cumulated CGROUP
restrictions.

So using a dedicated slice for running REDNODEs works
correctly because REDWRAP takes care of setting CGROUP
features with the hierarchy of nodes.

So we recommand to use the slice named `redpak` for
running the CGROUP aware REDNODE.

```
systemd-run --user -p Delegate=yes --slice redpak ...
```

or

```
systemd-run -p Delegate=yes --slice redpak ...
```



[1]: https://systemd.io/CGROUP_DELEGATION/
[2]: https://wiki.archlinux.org/title/Cgroups

