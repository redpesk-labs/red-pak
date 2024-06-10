# Overview of redpesk-labs/red-pak

.VERSION: 2.2.3

The component redpesk-labs/red-pak is here denoted as REDPAK.

## Overview

The software component REDPAK is a critical component exploiting
namespaces as offered by linux kernel for running applications
in a securized environment. Redpak is a light weithed container
container solution designed to run in constrained environment
for running embedded applications.

Within REDPAK, the term *rednode* refers to a named execution
environment defined by a configuration file.

Within REDPAK, rednodes can be nested in a simple hierarchical way
where one rednode, called the *child*, is setup on top of one other
rednode, called the *parent*, in a such way that restriction of
parents always apply to children.

REDPAK is integrated in the package manager of redpesk OS to enable
adding, removing and updating packages in rednodes. REDPAK is in
the package management of the system for optimally managing
dependencies of applications. The nested hierarchy of rednodes
forbids to install in children a package already installed in a parent.

And when one of its managed application is started, it setups
the linux's namespaces and cgroups and takes required actions
for running the application in its securized environment, its rednode
and the environment of all its parent's rednodes.

## Main use case

REDPAK is primarily intended to reenforce the security offered
by redpesk framework by strctly controling execution environment
of containerized applications.

This allows to run untrusted or poorly trusted applications
in an environment that forbids them anything that was not authorized.

So the main component to audit for proving trust is not the
application by its self but the configuration given to redpak
that defines it authorizations.


