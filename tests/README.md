
In order to run the tests, it might be necessary to export the below variables:

- REDNODE_TEMPLATE_DIR path to template configurations directory
- CHECK_BWRAP          path of bwrap binary
- CHECK_REDMICRODNF    path of redmicrodnf binary

for the plugin of RPM use a file containing:

%__plugindir PLUGINDIR
%__transaction_redpak %{__plugindir}/redpak.so" \

then either add it in file $HOME/.rpmmacros

or use export varaible RPM_CONFIGDIR=DIR
and in that DIR put a file containing
