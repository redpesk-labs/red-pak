
In order to run the tests, it might be necessary to export the below variables:

- REDNODE_TEMPLATE_DIR path to template configurations directory
- CHECK_BWRAP          path of bwrap binary
- CHECK_REDMICRODNF    path of redmicrodnf binary

For the plugin of RPM create a file containing:

%__plugindir PLUGINDIR
%__transaction_redpak %{__plugindir}/redpak.so

where PLUGINDIR is where the plugin is installed

and either names it $HOME/.rpmmacros

or use export variable RPM_CONFIGDIR=DIR
and in that DIR put the file
