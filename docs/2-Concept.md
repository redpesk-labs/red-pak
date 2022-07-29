# Concept

## Specification Illustration

![redpak concept](images/concept.svg "redpak concept")

### Control tools

- green part: provide control tools that offer linux and used in most of the container
- yellow part: represents the storage on disk that is used by the container in order to share resources with the system and also with parents. Indeed, redpak has a representation of a tree type parent-child.
- package manager: install/update/remove atomic by node

## Hierarchy

![redpak hierarchy](images/hierarchy.svg "redpak hierarchy")

### Tree structure

redpak has a tree structure of type parent-child
where each element is called a **node** or **rednode**
and can be a container.

### Gather resources

The idea is to gather resources between different containers
running different type of applications.

This scheme represents an example of a possible organization
of containers.
Different platforms are provides as redpesk or AGL
with different profiles inside like IVI or HTML5.
It also means that several versions of
a same library can be shared between brothers for example.

### Inherit security constraints: Russian Doll

Another idea is to inherit from security constraints from parents,
indeed redpak defines that each child cannot bypass its parent constraints.
In fact, redpak can be seen as a behavior of russian doll.

## Rednodes

![redpak nodes](images/nodes.svg "redpak nodes")

A rednode is an element of the hierarchy,
on this example, there are platform, profile, project nodes.

On the left of the scheme, on disk, it is stored as a rootfs snippet following a filesystem tree.
Inside the container, everything is remounted to a flat architecture. The slash
is in tmpfs and system or parent dependencies are remounted into specific
directories.

And all this is entirely configurable with config yaml files.

## Rpm databases

![redpak rpm databases](images/rpmdatabases.svg "redpak rpm databases")

Another key point of redpak is the package manager that allow to install rpm packages inside of a node.
For that, each node get its own rpm database and an installation
is done by node atomically.

A basic principle of redpak is the resources shared, at a package level
it means that package installed in a node cannot be installed on a child.

So, redpak introduce a database aggregation, that is to say
an installation inside is done with the sum of parent databases.

Finally, the installation of a pkg is quite standard from
a classic package manager with standard commands like dnf with an
additional option as the redpak, which defines where is stored you node,
for example:

```bash
redwrap-dnf --redpath /var/redpak/<MYNODE> install <MYPKG>
```

## Performance

![redpak performance](images/performance.svg "redpak performance")

These measurements were done on 3 type of board: a qemu, a NXP board, and
a xilinx, the ultrascale+.

Here there are 2 types of performance,
the first one is the starting time which is important on embedded systems,
especially where there is large constraint on startup time.

Each container launches `cat /dev/null` a 100 times. For each iteration, the time is recorded before and after the
command and this is the difference you see on the table.

the starting time is very interesting for embedded and redpak was also meant
to do so, so it is satisfying

The second measurement is about the memory usage

for that, Each container launches multiple processes. For each of them, the amount of used memory of their processes has been added up to
get the total used memory by the engine of the container. The memory consumption was given by the `ps auxc` command.

The memory usage is quite similar to lxc so it is quite good, but there some points to be careful, each level adds some times and memory
(these tests were done at the first level).

Some other feature like execfd mount mode can be very greedy so it really depends on configuration too, for these measurement it was done with the default configuration.:w

## Conclusion

REDPAK is designed to match constraints of embedded world, by sharing resources with not only systems but also parents.
It is also meant to be totally manageable with rpm packages.
Finally, in a security aspect, it is totally auditable and works as a white box.

Redpak is still in development and the sources are available in github and can be directly usable with redpesk OS by installing the following rpm package.

```bash
dnf install red-pak
```
