<!---
include yaml files
![main system](images/main-system.txt "main system")
![default](images/default.txt "default")
-->

# Yaml configuration

## Sections

In yaml configuration file, there are 4 sections:

- Headers: rednode information
- Config: node configuration (namespaces, cgroups, ...)
- Exports: node mounts
- Environ: node environment variables

There parameters are merged in rednode hierarchy.

## Yaml System configuration

Below, there is the template of yaml configuration file

<embed type="text/plain" src='images/main-system.txt' width="100%" height="500">
<a href="https://github.com/redpesk-labs/red-pak/blob/master/etc/redpak/templates.d/main-system.yaml">main-system.yaml</a>
</embed>

## Yaml defaut configuration

<embed type="text/plain" src='images/default.txt' width="100%" height="500">
<a href="https://github.com/redpesk-labs/red-pak/blob/master/etc/redpak/templates.d/default.yaml">default.yaml</a>
</embed>

## Good Practises


