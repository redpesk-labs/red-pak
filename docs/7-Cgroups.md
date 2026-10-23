# Using cgroups with redpak and systemd

redpak has options to setup cgroup restrictions on started
rednodes. This behaviour duplicates the behaviour of
systemd that already manages cgroup features.

One of the design principle of how systemd manages cgroup
is that only one writer can control a cgroup (see [Two Key Design Rules][1]).

That **single-writer** behaviour can not be enforced by the
kernel. So designers of systemd took the decision that
the full cgroup hierarchy is under the control of systemd
except if the opposite is explicitly required.

Using cgroup features of redpak without taking care of what
systemd expects is possible but:

- it violates the **single-writer** principle because
  redwrap will write and rearrange features in cgroup
  when systemd expects to be the only writer.

- it implies that you run redwrap as root or with some
  capabilities for writing where systemd writes.

In order to solve this kind of issues, systemd has
provision for allowing other writers to manage parts
of cgroup that system will not overwrite after setting it up.
This behaviour, called **delegation**, is described in
that [document][1].


## Delegation in systemd's user land

The two lines below are from systemd's service `user@.service`

```
Slice=user-%i.slice
Delegate=pids memory
```

It shows that every user service or application started by
systemd and the children of these services or applications
are in the cgroup slice `user.slice/user-UID.slice/`.

And that slice is restricted to have only access to the
cgroup's controllers `pids` and `memory`.

So if the rednode only needs to setup `pids` and `memory`,
it could be possible in theory, because the controllers
are available. But in fact it will not work because the
files controlling the behaviour of these cgroups are owned by
root and are write protected.

Before discussing on how systemd allows delegation of the
control of these features, let's talk about other cgroup
controllers that redpak manages: cpu, cpuset and io.

Because these controllers are not listed in `user@.service`,
they cannot be delegated to user services. This can be
changed by adding drop-in configurations. For example
for adding the missing controllers to user 1001, it is
needed to create the file
`/etc/systemd/system/user@1001.service.d/delegate.conf`
([more details here][2]) that contains:

```
[Service]
Delegate=cpu cpuset io
```

But this action requires system privileges to be achieved.

Without delegation, the cgroup properties can be controlled
by systemd settings and properties.

In order to run redpak in user mode, you need to turn on the
delegation. This is done by including the below lines in your
service:

```
[Service]
Delegate=yes
```

that delegates all available cgroup controllers.

Or by listing the expected cgroup controllers like below:

```
[Service]
Delegate=memory pids
```

In this cases, systemd creates a slice for controlling
the service. That slice can be named explicitly using
`Slice` directive of systemd.

When running redpak from command line in the user environment,
if the rednode has cgroup settings, they may be applied or not
depending on rights that systemd lets to cgroups files. But
in all cases it conflicts with systemd and the **single-writer** rule.

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

In this case, the user still gets the ability to
modify its cgroup restrictions itself. This is intended
for implementing a manager but not for running
a restricted program. However redpak allows running
rednodes unable to change their cgroup restrictions
if the cgroup namespace is unshared:

```
config:
    share_cgroup: disabled
```


## Delegation in systemd's system land

Restriction applied by `user@.service` are not existing
in system land. So all cgroup controllers are available.

But the use of the directive `User`, even in system's services,
makes them user services and then only allows *pids* and *memory*
controllers as seen above. To circumvent this
restriction, either follow what is explained above or use
the redpak feature for changing the user.

Since redpak version 2.4.2, it is possible to start
a rednode from root that will run as another user.
It opens the possibility to use all cgroups from
system land, but in user land. In such a case, services
could be changed to not set `User=...` while still
setting dependency to `user@%i.service`. That way,
user services are started and all cgroup controllers are
available.

When using this option, items of cgroup hierarchy are
owned by root, not by the end user. This is good because
it avoids the rednode to change its settings.


## Security considerations on using cgroup and delegation

When delegation of cgroup is granted by systemd to a
service, the service is allowed to change its cgroup
settings. The hierarchical imbrication of the cgroups
means that it can not be better than what systemd
allocated initially to the main service.

But it means that if the service forks, it has to
be cautious of what its children, running with the
same rights, can change in the cgroup settings.

This can be avoided by un-sharing the cgroup namespace
in the redpak configuration file (`config.share_cgroup: disabled`),
this ensures that the settings of rednode configuration
will not be changed.

## rednodes, cgroups, and russian dolls

For cgroup management of rednode, the concept
of russian dolls applies. The hierarchy of rednodes
is applied by fitting children in their parent.
This behaviour automatically applies with cumulated cgroup
restrictions.

So using a dedicated slice for running rednodes works
correctly because redwrap takes care of setting cgroup
features with the hierarchy of nodes.

So we recommend to use the slice named `redpak` for
running the cgroup aware rednode. But note well that
using the default slice `user-%i` also works.

```
systemd-run --user -p Delegate=yes --slice redpak ...
```

or

```
systemd-run -p Delegate=yes --slice redpak ...
```

[1]: https://systemd.io/CGROUP_DELEGATION/
[2]: https://wiki.archlinux.org/title/Cgroups

