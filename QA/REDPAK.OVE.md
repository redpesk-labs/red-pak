# Overview of redpesk-labs/red-pak

.VERSION: 2.2.3

The component redpesk-labs/red-pak is here denoted as *REDPAK*.

## Overview

The software component *REDPAK* is a critical component exploiting
namespaces as offered by linux kernel for running applications
in a securized environment. *REDPAK* is a lightweight container
solution designed to run in constrained environment for running
embedded applications.

Within *REDPAK*, the term *REDNODE* refers to a named execution
environment defined by a configuration file. A *REDNODE* might
have private data or data shared with other parts of the system,
including *REDNODE*s. In other words, a *REDNODE* is a light container.

Within *REDPAK*, *REDNODE*s can be nested in a simple hierarchical way
where one *REDNODE*, called the *child*, is setup on top of one other
*REDNODE*, called the *parent*, in a such way that restriction of
parents always apply to children.

*REDPAK* is integrated with a package manager in redpesk OS to enable
adding, removing and updating packages in *REDNODE*s. Doing so allows
to install in *REDNODE*s specific components that are not available
for the system or for other nodes. It also allows to install different
versions of components in different *REDNODE*s.

Within *REDPAK*, the term *REDWRAP* refers to the component used for
setting up the *REDNODE* and running commands within that *REDNODE*.
*REDWRAP* sets up linux's namespaces and cgroups and takes required
actions for running the application in that isolated environment,
that *REDNODE*.

## Main use case

*REDPAK* is primarily intended to reenforce the security offered
by redpesk framework by strictly controlling execution environment
of containerized applications.

This allows to run untrusted or poorly trusted applications
in an environment that forbids them anything that was not authorized.

So the main component to audit for proving trust is not the
application by itself but the configuration given to *REDPAK*
that defines its authorizations.
