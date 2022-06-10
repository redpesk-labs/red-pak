# Troubleshooting

## Smack

### Labels with vim

When you open a file with vim and save it, vim creates a copy of it and replace the original by the copy,
and the label on the file can be lost.

Use `chsmack` command to relabel the file.

## Cgroups

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