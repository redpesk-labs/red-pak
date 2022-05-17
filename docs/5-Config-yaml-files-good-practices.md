# Config yaml file good practices

The purpose of this part is to give advices to write a config yaml file to
configure your nodes within the hierarchical view.

There are 4 parts in node config, and default yaml templates
can be seen here: ![](3-Configuration).

Note that a node is totally manageable through the yaml config files and
so directly represents the dynamic node behavior.

## Redconf tools

`redconf` tools is a tool that will parse and provide information about a node yaml config file.

The 2 modes can be useful to fix grammar issues with `redconf config` or to see what overloaded or getting the cumulated/merged config of a node with `redconf mergeconfig`.

See []() section for more information about redconf.

## Headers part

The headers are automatically provided at the node creation, it mainly gives
information about the node.

You can see example in default yaml file.

## Config part

The config part defined constraints and mandatory values of the nodes.

Some values are cumulated as the `path` of `ldpath` other are overloaded by children,
typacally some proper value of a node as `hostname`.

Eventually, some values cannot be overloaded, they defined security constraints as namespace values.

### Constraint values

Here, the constraint behavior is explained within the node hierarchy.

#### Namespaces

redpak handles all linux namespaces, see man documentation: [https://man7.org/linux/man-pages/man7/namespaces.7.html](https://man7.org/linux/man-pages/man7/namespaces.7.html)

* share_user
* share_cgroup
* share_net
* share_pid
* share_ipc
* share_time

There are 3 states for these values:

* `disabled` : means that a new namespace is created and none of the children can share it
* `enabled` : means that the namespace is shared and children are allowed to disable it
* `unset` : means that the disabled/enabled is authorize in children (default is disabled: meaning that if all node in the family is `unset` the namespace is disabled)

#### Capabilities

