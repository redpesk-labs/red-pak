# Concept

## Specification illustration

![redpak concept](images/concept.svg "redpak concept")

### Control tools

- green part: provides control tools offered by linux and used in most parts of the container
- yellow part: represents the storage on disk that is used by the container, in order to share resources with the system and also with parents. Indeed, redpak uses a parent-child type tree representation.
- package manager: install/update/remove, atomic by node.

## Hierarchy

![redpak hierarchy](images/hierarchy.svg "redpak hierarchy")

### Tree structure

redpak has a tree structure of type parent-child
where each element is called a **node** or **rednode**
and can be a container.

### Gather resources

The concept is to gather resources between different containers
running different types of applications.

The previous schema represents an example of a possible organization
of containers.
Different platforms are provided, like redpesk and AGL, with different
embedded profiles, like IVI and HTML5.
It also means that several versions of
a same library can be shared between brothers for example.

### Inherit security constraints: Russian Doll

Another concept is to inherit security constraints from parents.
Indeed redpak defines that a child cannot bypass its parent constraints.
In fact, redpak can be seen as having the behaviour of russian dolls.

## Rednodes

![redpak nodes](images/nodes.svg "redpak nodes")

A rednode is an element of the hierarchy.
On this example, there are platforms, profiles, and project nodes.

On the left of the schema, the container is stored as a rootfs snippet following a filesystem tree.
Inside the container, everything is remounted to a flat architecture. The / directory
is located in tmpfs, and system or parent dependencies are remounted into specific
directories.

All of this is entirely configurable with yaml configuration files.

## Rpm databases

![redpak rpm databases](images/rpmdatabases.svg "redpak rpm databases")

Another key point of redpak is the package manager that allows installing rpm packages inside of a node.
To do so, each node gets its own rpm database and installation
is done atomically by node.

A basic principle of redpak is that resources are shared, at a package level.
It means that a package installed in a node cannot be installed on a child.

So, redpak introduces a database aggregation, that is to say
an internal installation including all of parent databases.

Finally, the installation of a package is quite similar to the one of
a classic package manager, with standard commands like dnf, with an
additional option which defines where the node is stored.
For example:

```bash
redwrap-dnf --redpath /var/redpak/<MYNODE> install <MYPKG>
```

## Performance

![redpak performance](images/performance.svg "redpak performance")

These measurements were done on 3 types of board: a qemu, an NXP board, and
a xilinx (the ultrascale+).

There are 2 types of performance measurements.
The first one is the starting time which is important on embedded systems,
especially when there is a constraint on startup time.

Each container launches `cat /dev/null` 100 times. For each iteration, the time is recorded before and after the
command, and the difference between the two times is shown in the table.

The startup time is very important for embedded systems, and redpak was meant
to behave well in this domain.

The second measurement is about memory usage.

Each container launches multiple processes. For each of them, the amount of used memory of their processes has been added up to
get the total memory used by the engine of the container. The memory consumption was given by the `ps auxc` command.

The memory usage is quite similar to the one of lxc, so it is quite good. But be warned that each additional level increases time and memory consumption
(these tests were done at the first level).

Other features, like execfd mount mode, can be very greedy, so memory consumption really depends on the configuration.
The measurements were done with the default configuration.

## Conclusion

redpak is designed to match constraints of embedded world, by sharing resources with not only systems but also parents.
It is also meant to be totally manageable with rpm packages.
Finally, in a security aspect, it is totally auditable and works as a white box.

redpak is still in development and the sources are available in github and can be directly used within redpesk OS by installing the `red-pak` package:

```bash
dnf install red-pak
```
