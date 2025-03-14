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

<iframe src="yaml/root-normal.txt" width="100%" height="800"/>

## Yaml default configuration

<iframe src="yaml/leaf-normal.txt" width="100%" height="500"/>

## Good Practices

To see good practices to write yaml config node file see [Good practices]({% chapter_link redpak.config-yaml-files-good-practices %})
