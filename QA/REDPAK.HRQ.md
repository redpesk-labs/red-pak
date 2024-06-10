# High level requirements of redpesk-labs/red-pak

.VERSION: 2.2.3

.AUTHOR: José Bollo [IoT.bzh]

.AUDIENCE: ENGINEERING

.DIFFUSION: PUBLIC

.git-id($Id$)

The component redpesk-labs/red-pak is here denoted as REDPAK.

## Overview

Here is a short introduction. For a more detailed overview
see the document @REDPAK.OVE.

Within REDPAK, the term *rednode* refers to a named execution
environment defined by a configuration file.

Within REDPAK, *rednodes* can be nested in a simple hierarchical way
where one *rednode*, called the *child*, is setup on top of one other
*rednode*, called the *parent*, in a such way that restriction of
parents always apply to children.

REDPAK is integrated in the package manager of redpesk OS to enable
adding, removing and updating packages in *rednodes*. The nested
hierarchy of *rednodes* forbids to install in children a package
already installed in a parent.

## Requirements

### Rednode


### Wrapper program for entering rednodes

.REQUIREMENT

REDPACK provides a program called the *wrapper* that must be used for
executing programs in a specific *rednode*.

**MOTIVATION**:

This is mandatory because this is the only way offered by linux of running
a program in a controled environment. The executed program naturaly *inherit*
the settings of its parents. REDPAK *wrapper* is invoked for setting the
restricted environment of the process to be executed.


### Conformance to rednode configuration

.REQUIREMENT

The settings 
REDPAK *wrapper* must apply the settings of *rednode* for the program
it runs. These  configuration as
described by the document @REDPAK.CONF

**MOTIVATION**:

Designers of systems are only dealing with configuration files.

