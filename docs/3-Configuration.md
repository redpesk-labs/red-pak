# Yaml configuration

## Sections

A yaml configuration file includes 4 sections:

- Headers: rednode information
- Config: node configuration (namespaces, cgroups, ...)
- Exports: node mounts
- Environ: node environment variables

These parameters are merged in rednode hierarchy.

## Yaml System configuration

Here below is the template of yaml system configuration file:

<iframe src="yaml/root-normal.yaml" width="100%" height="800"/>

## Yaml default configuration

<iframe src="yaml/leaf-normal.yaml" width="100%" height="500"/>

## Good Practices

Good practices to write a yaml config node can be checked [here]({% chapter_link redpak.config-yaml-files-good-practices %}).
