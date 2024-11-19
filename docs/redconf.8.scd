redconf(8)  "Redpesk OS"

# NAME

redconf - RedNode configuration manager

# SYNOPSIS

*redconf* [<opts>] command [<args>]

# DESCRIPTION

*redconf* is an administrative tool for creating or inspecting rednodes.

The commands are:

- *create* for creating a rednode,
- *config* for getting specific rednode config,
- *mergeconfig* for getting final rednode config,
- *tree* for getting the tree of rednodes heriarchy.

# GLOBAL OPTIONS

*-v, --verbose*
	Increase the verbosity. Can be repeated. Can optionaly be followed
	by a number terlling the expected verbosity level, 0: EMERGENCY,
	1: ALERT, 2: CRITICAL, 3: ERROR, 4: WARNING, 5: NOTICE, 6: INFO,
	7: DEBUG, 8: TRACE.

*-y, --yaml*
	YAML output where meaningful.

*-l, --log-yaml* _LEVEL_
	Log level of libcyaml when reading and writing YAML data.

*-h, --help*
	Show help and exit

# COMMAND CREATE

The command *create* installs a new rednode. It copies the configuration
files (normal and admin) and set up the rednode status file.

When used with option *--update* the command *create* accepts to update
the configurations of an existing node instead of creating it.

## Synopsys of command create

*redconf* *create* <opts> redpath

## Options of command create

*-a, --alias* _NODE-ALIAS_
	Set the name of the rednode. If not given, the basename of the
	node directory is used.

*-c, --config* _CONFIG-FILE-OR-MODEL_
	Indication for locating the normal configuration file.
	See below *LOCATION OF CONFIG FILES*.

	Default is *leaf*.

*-C, --config-adm* _CONFIG-FILE-OR-MODEL_
	Indication for locating the admin configuration file.
	See below *LOCATION OF CONFIG FILES*.

	Default is *leaf*.

*-t, --templates* _TEMPLATE-DIR_
	Selects an alternate template directory.
	See also environment variable *redpak_TMPL*.

*-u, --update*
	Update an existing rednode.

*-f, --full*
	Shortcut for options *--config full --config-adm full*.

*-l, --leaf*
	Shortcut for options *--config leaf --config-adm leaf*.

*-r, --root*
	Shortcut for options *--config root --config-adm root*.

*-m, --model* _MODEL_
	Shortcut for options *--config MODEL --config-adm MODEL*.

*-n, --node* _REDPATH_
	The path of the node to create. This option is redundant with the
	first positional argument.

*-R, --redroot* _ROOT-PATH_
	The root of rednodes. Default is */*.

*-h, --help*
	Print help on create command usage.


# COMMAND CONFIG

The command *config* outputs the specific configuration of one node.
The specific configuration is the configuration of that node, outside
of any node hierarchy Specific. It means that configuration items coming
from parent nodes hierarchy are not shown by this command (but see
command mergeconfig)

## Synopsys of command config

*redconf* *config* <opts> redpath

## Options of command config

*-r, --redpath* _REDPATH_
	The path of the node whose config is to be displayed.
	This option is redundant with the first positional argument.

*-h, --help*
	Print help on config command usage.


# COMMAND MERGECONFIG

The command *config* outputs the merged configuration of one node.
The merged configuration is the final configuration of that node,
taking into account configurations inherited from parent nodes
hierarchy.

## Synopsys of command mergeconfig

*redconf* *mergeconfig* <opts> redpath

## Options of command mergeconfig

*-e, --expand*
	Expands the expandable variables on output.

*-r, --redpath* _REDPATH_
	The path of the node whose config is to be displayed.
	This option is redundant with the first positional argument.

*-h, --help*
	Print help on mergeconfig command usage.

# COMMAND TREE

The command *tree* displays the tree rednodes starting
from a path in rednode hirarchy.

## Synopsys of command tree

*redconf* *tree* <opts> redpath

## Options of command tree

*-r, --redpath* _REDPATH_
	The path of the node whose config is to be displayed.
	This option is redundant with the first positional argument.

*-h, --help*
	Print help on config command usage.

# LOCATION OF CONFIG FILES

When an argument _ARG_ is given to options *--config* or *--config-adm*,
the below paths are tested in order and the first existing file is used:

. ${PREF}_ARG_
. ${PREF}_ARG_.yaml
. ${PREF}_ARG_-${TYPE}.yaml

With the below values for *${PREF}* and *${TYPE}*:

- If _ARG_ has a slash, it should be a file, so *${PREF}* is the
  empty string. Otherwise, it should be a temùplate indication
  so *${PREF}* is substituted with the path of templates' directory.

- The value of *${TYPE}* is either *normal* or *admin*, depending on
  the option (*--config* or *--config-adm*).

For example, if option given is *--model tiny*, then normal config
is searched in:

. */etc/redpak/templates.d/tiny*
. */etc/redpak/templates.d/tiny.yaml*
. */etc/redpak/templates.d/tiny-admin.yaml*

Adn normal config in:

. */etc/redpak/templates.d/tiny*
. */etc/redpak/templates.d/tiny.yaml*
. */etc/redpak/templates.d/tiny-normal.yaml*


# ENVIRONMENT

The below ernvironment variables can be used.

*REDNODE_TEMPLATE_DIR*
	Location of the template configurations directory.
	Default value: *"/etc/redpak/templates.d"*

# EXAMPLES

This example show the creation 
```
> redconf create -m root /tmp/root -a ROOT
> redconf create /tmp/root/leaf1 -a LEAF-1
> redconf create /tmp/root/leaf1/leaf11 -a LEAF-1-1
> redconf create /tmp/root/leaf1/leaf12 -a LEAF-1-2
> redconf tree /tmp/root
/tmp
  └── root  (ROOT)
      └── leaf1  (LEAF-1)
          ├── leaf11  (LEAF-1-1)
          └── leaf12  (LEAF-1-2)
```

It is possible to get the configuration of one of these node:

```
redconf -y mergeconfig -e /tmp/root/leaf1/leaf12
```

# SEE ALSO

*redwrap*(8), *rednode*(5)

# AUTHOR

IoT.bzh
