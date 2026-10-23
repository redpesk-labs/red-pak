# Troubleshooting

## Systemd Notify

If you want to use systemd to start your node and be notified, you can use
`NotifyAccess=all`, it will accept notifications from a different pid than the one
launched by the unit.

### notify vs cgroup

When systemd daemon receives a message through the socket, it searches the
unit matching the cgroup of the process, so if you use cgroup with redpak,
redpak cgroup should be a child of the systemd unit cgroup.

For that, you can just remove the `cgrouproot` option in configuration files.

See [https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#NotifyAccess=](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#NotifyAccess=)

## SMACK

### Labels with vim

When you open a file with vim and save it, vim creates a copy of it and replaces the original by the copy,
and the label on the file can be lost.

Use `chsmack` command to relabel the file.

## cgroups

You can visit [a specific chapter for correctly configuring and using cgroup]({% chapter_link redpak.using-cgroups %})

### V2 Issue

Redpak handles cgroups only in pure version 2. If you are in cgroup version 1, or hybrid, you need to reboot after appending to the command line:

```bash
systemd.unified_cgroup_hierarchy=1
```

After reboot, you can check that `/sys/fs/cgroup' is in version 2 with:

```bash
mount -l | grep /sys/fs/cgroup
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```

### User issue

If you don't have the rights to create a sub cgroup into your current parent cgroup, it may be due to the fact that you're not in a user cgroup session.

You can try to start one with:

```bash
# rp-owner
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export DBUS_SESSION_BUS_ADDRESS=unix:path=${XDG_RUNTIME_DIR}/bus
systemd-run --user -p "Delegate=yes" --slice redpak --shell
```

### Controller issue

If you can't write into some controllers, it may be due to an issue in delegation. For that, you need to verify:
- /sys/fs/cgroup from your parent cgroup (`cat /proc/self/cgroup`),
- the available controllers `cat cgroup.controllers`,
- the controllers delegated to the children `cgroup.subtree_control`.

A temporary way to test it, is to append the missing ones at each level, for example:

```bash
echo "+cpuset +cpu +memory +io +pids" > cgroup.subtree_control
```

Then you can check in the child that you can see the controllers:

```bash
# in child
cat cgroup.controllers
```

All of the elements of redpak configuration file respects kernel definitions, please have a look at
[https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html).


