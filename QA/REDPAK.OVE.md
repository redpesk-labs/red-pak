# Overview of redpesk-labs/red-pak

.VERSION: 2.4.1

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ALL

.DIFFUSION: PUBLIC

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as *REDPAK*.

## Overview

The software component *REDPAK* is a critical component exploiting
namespaces as offered by linux kernel for running applications
in a isolated environment. *REDPAK* is a lightweight container
solution designed to run in constrained environment for running
embedded applications.

Within *REDPAK*, the term *REDNODE* refers to a named execution
environment defined by a configuration file. A *REDNODE* might
have private data or data shared with other parts of the system,
including other *REDNODE*s. In other words, a *REDNODE* is a light container.

Within *REDPAK*, *REDNODE*s can be nested in a simple hierarchical way
where one *REDNODE*, called the *child*, is setup on top of one other
*REDNODE*, called the *parent*, in a such way that restrictions of
parents always apply to children.

*REDPAK* is integrated with the package manager *red-microdnf* in
redpesk OS to enable adding, removing and updating packages in *REDNODE*s.
Doing so allows to install in *REDNODE*s specific components that are not
available for the system or for other nodes. It also allows to install
different versions of components in different *REDNODE*s.

![Figure: redpak environment](assets/REDPAK-fig-interfaces.svg)

Within *REDPAK*, the term *REDWRAP* refers to the binary launcher used
for setting up the *REDNODE* and running commands within that *REDNODE*.
*REDWRAP* sets up linux's namespaces and cgroups and takes required
actions for running applications in that isolated environment,
that *REDNODE*.

In its implementation, *REDWRAP* uses the software component *bubblewrap*
(whose binary is named *bwrap*) and the library *cyaml*.

## Goals of REDPAK

Lightweigtht containerizing with *REDPAK* matches the following goals:

- Ensuring isolation of applications
    - Restricted visibility of files
    - Restricted ressources: CPU, Memory, I/O, Network, Services, ...
    - Integrated with in kernel MAC (Mandatory Access Control)

- Being auditable with ease
    - Avoid classical "black box" containers
    - Configuration easy to audit even by humans

- Maximizing sharing and minimizing system overload
    - not duplication of root-fs
    - Restricting RAM, Disk and UCC containerization Costs
    - Reusing shared libraries between instances
    - Improved container startup time


## Main use cases

*REDPAK* is primarily intended to re-enforce the security offered
by redpesk framework by more strictly controlling execution environment
of applications.

This allows to run untrusted or poorly trusted applications
in an environment that forbids them anything that is not authorized.

Consequently, when an application is running in a *REDNODE*,
the main component to audit for proving trust is not the
application by itself but the configuration given to *REDPAK*
that defines granted authorizations.

*REDPAK* can also be used for deploying different versions of libraries
and applications in separated environments. Such behaviour can be
useful for smooth upgrades.
