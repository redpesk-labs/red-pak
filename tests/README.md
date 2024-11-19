# Content

This directory contains 2 unitary tests, a set of basic
tests and a set of live tests.

## check.c

This test needs redmicrodnf.

Requires special setting (see below) when test
environment isn't standard.

## check-conf.c

This test checks some basic functions of library
redconf.

Requires special setting (see below) when test
environment isn't standard.

## directory basic

This directory contains a test script that check
how rednode stacking affects the arguments passed
to to bubble wrap launcher *bwrap*.

## directory conf

This directory contains a test script that check
how if configuration files are correctly parsed.

## directory live

This directory contains a test script and its
data for checking that running environments as setup
by redwrap is conforming expectation.

Live tests are requiring the following packages
to be installed:

- busybox
- gcc
- glibc-static

## Setting test environment

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
