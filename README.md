    ---
    redpak an Ultra Light Weight Container for embedded applications.
    ---

Work In progress: binary packages should come soon. In the mean time early adopter should start from source code.

 - Videos:
    + Introduction to concepts and architecture https://vimeo.com/412131602
    + Quick demo on how to install AGL framework and helloworld demo https://vimeo.com/412135018

- redpak target embedded and critical infrastructures
    + it maximizes resource sharing (no rootfs/sharedlib duplication)
    + it is designed to be auditable. While each individual nodes are independent, the global coherency is provided by dnf/rpm and libsolv at the coreos level). redpak coherency can be statically proven at CI level before pushing the image to the target.
    + it simplifies container inspection (a node is an atomic subset of a rootfs)
    + it uses standard management tools (dnf+rpm)
    + built with long term support and cybersecurity in mind.

- Main differences with other existing containers (Docker, LXC, Snap, Flatpack, ...)
    + (Docket, LXC, ...) are mostly IT/Datacenter oriented containers 
        + They duplicate rootfs
        + They generally run or at least start as privileges users
        + They share as less possible resources with the host (to provide portability)
        + They have a lot tools to handle Devops operations (backup, migration, elasticity, ..)
    + (Snap, Flatpack, Appimage, ...) Desktop driven containers 
        + Except for Flatpack they also duplicate rootfs
        + Like redpak they run without privilege
        + Because they target easiness of application installation, they cannot maximum resource sharing with coreos host.
        + They have less tools (mostly) application store) and try to appear on your system as standard applications.
        + The main goal is to address Linux fragmentation, allowing the same binary to run on every Linux flavour/version.
    + (redpak) Managed driven system for the embedded world
        + Embedded and generally every critical systems are fully managed. Every configuration and combination should be tested before getting to production. The fact your application SHOULD work with ALMOST every configuration without having to be tested is not good enough.
        + Embedded target are generally limited in resources (RAM,CPU,DISK,POWER,...) Duplicating library/tools on disk/memory is not an option.
        + Embedded system need to be auditable/certifiable. A black box container model is not acceptable. 

    Everyone understand that installing a software component on millions of cars, on a submarine or in a train is very different from installing a new application on a desktop or a phone.

    redpak target embedded devices used within vital infrastructure (automotive, boat, submarine, train, civil infrastructure, medical, ...). redpak does not use black box containers, on the opposite it enforces a white box model where the global coherency of the system can be proven. While each node of a redpak family own an atomic subset of the full tree rootfs, the global coherency of each node is statically verified. Installation/update and be proven before installation on the embedded target by an adequate CI/Build-system as Redpesk (http://docs.redpesk.bzh).

---
    The red-family:

Tools:

    - red-dnf: handles nodes live cycle (target development time)
        + creation
        + adding packages repositories
        + install/update packages
        ...

    - red-rpm: provides rpm management on target. It is also install packages coming 'Over the Air' update.

    - red-wrap: creates execution namespace.

Concept:

    - RedNode: the atomic component of redpak. A node is an independent subset of full tree rootfs. Each node owns a private RPMdb and repository lists. Node can be installed or updated independently. Each Rednode owns:
        + Alias: should be unique within your Redpath, but not globally (i.e. two version of QT may have the same alias)
        + Name: it should be unique. It is used for debugging purpose (very useful when multiple nodes share the same alias)
        + Config: hold environment, path, flag, ... for corresponding node
        + Exports: a list a directories/files a node imports/exports.

    - Redpath: represent a family of nodes that are imported in a namespace at application launch time. Every action on redpak is done by providing the path to the terminal node of its family tree. 

    - Redmain: the coreos and default configuration. Mostly exposes default values for nodes at run time.

    - Exports: depending on the mode the visibility is full/limited on part/all the tree family
        + Public: share point is visible in RW from every children.
        + Restricted: Idem but only visible in RO.
        + Private: Visible only from application running in the same node (not from children).
        + Anonymous: not visible outside current name space (even two applications running in the node have separated namespace)
        + ExecFd: phony file that contain the result of a shell command (i.e: one line of /etc/passwd)
        + Symlink: symbolic link 
        Note: mount may indifferently import directories or files.

    - Share/unshare:
        + List of tag to expose network, pid, users, ...

    - Environment:

        + Static export fix key/value 
        + Expanded replace $VALUE before export key/value (i.e. Node_Alias, UID, ...)
        + ExecFd export the stdout of a shell command
        + Remove remove a variable before entering in the application namespace

    - Underlying technologies

        - dnf/lib dnf: 
            - Unfortunately as today redpak require a small patch on libdnf (~100 lines)
            - Uses a custom main CLI to set proper arguments
            - Ship a C++/Python module that is used by red-plugins
        - rpm:
            - No change on librpm.so only need librpm-devel to compile
            - Ship a custom version of librpm/python interface
            - Ship a custom red-rpm cli that understand --redpath 
        - bublewrap:
            - No change on bwrap
            - As today red-wrap exec bwrap command (in the future we could change for a direct call)
---
    Performances

    More test need to be done, with many different conditions/configuration (target, versions, ...) 
    - On my home laptop sample scenario with:

        - 3 nodes (redpesk-dev/agl-plateform/agl-demos-> demo->applications)
        - 25 mount (resources imports)
        Namespace Star/Stop time ~50-60ms (depends on global system load)

    - To check on your own system you can use

    ```
        TIMEMS=`echo "- $(($(date +%s%N)/1000000))  + "  && red-wrap --rpath=/var/redpesk/agl-redpesk9/agl-demo -- cat </dev/null ; echo $(($(date +%s%N)/1000000))`; echo $(($TIME))
    ```    

    Do not forget to retreive time mesurement cost

    ```
        OVERLOAD=`echo "- $(($(date +%s%N)/1000000))  + " ; echo $(($(date +%s%N)/1000000))`; echo $(($TIME))
    ```
---
    Quick Start

    0) Create a non privilege pool to host your Rednodes 

        + sudo mkdir /var/redpesk
        + sudo chown $USER /var/redpesk

    1) Create your 1st node 
    
        + red-dnf --redpath=/var/redpesk/agl-redpesk9 red-manager --create --alias=agl-core

    2) Add a repository of rpm packages to your node

        +  red-dnf --redpath=/var/redpesk/agl-redpesk9 red-manager --add-repo http://kojihub.lorient.iot/kojifiles/repos/II--RedPesk-9-build/latest/x86_64   

        Note: you may have multiple repository per node. (should be fixed) My sample URL is not public 

    3) Install a package in your node

        + red-dnf --redpath=/var/redpesk/agl-redpesk9 red-search binder
        + red-dnf --redpath=/var/redpesk/agl-redpesk9 red-install agl-app-framework-binder

    4) Start an application in your node.

        + red-wrap --redpath=/var/redpesk/agl-redpesk9 afb-daemon --version 
        Note: a this level we only have one node. If you want to compare with other existing containers technologies, you should thing of redpak container as Russian doll. Your container is not one node, but the imbrication of all nodes contain in your Redpath. i.e: node:coreos/node:agl-plateform/node:boat-midleware/node:my-projet->my-navigation-application

    5) Enter with 'bash' in your container

        +  red-wrap --redpath=/var/redpesk/agl-redpesk9 --force bash 
        ```
            > ls /
            > ls /nodes/agl-core/usr/bin/
            > afb-daemon --version
            > ldd /nodes/agl-core/usr/bin/afb-daemon
            > rpm -qa
        ```

    6) Add a new leaf to our tree.

        + red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-manager --create --alias=agl-demo

    7) Add a repository to your agl-demo node

        +  red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-manager --add-repo http://kojihub.lorient.iot/kojifiles/repos/IIExtra--RedPesk-9-build/latest/x86_64/

    8) Install helloworld demo

        +  red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-search agl
        +  red-dnf --redpath=/var/redpesk/agl-redpesk9/agl-demo red-install agl-service-helloworld.x86_64

    9) Enter your new node

        + ls /nodes
        + ls ls /nodes/agl-core/usr/bin/afb-daemon
    
        +  red-wrap --redpath=/var/redpesk/agl-redpesk9/agl-demo --force bash 
        +  afb-daemon --ldpaths=/nodes/agl-demo --workdir=. --roothttp=/nodes/agl-demo/usr/agl-service-helloworld/htdocs --verbose

        + browser http://10.20.101.105/1234

    10) delete a node (they are atomic, a simple 'rm' is enough)

        + rm -rf /var/redpesk/agl-redpesk9/agl-demo

---
    Config:

+ main config is at /xxx/etc/redpak/main.yaml
+ node config is at $NODE_PATH/etc/redpack.yaml

Note on config: All sections but 'config/tags' cumulate. Every exports path, mounting point or environment variables defined within each nodes of a given redpath are visible at runtime from the application namespace. Config/tags are unique and fusion at run time. Tags defined within redmain.yaml are used as default values. Then within redpath, the oldest ancestor has the priority. As the result if a parent enforce a constrain children cannot overload the selection (i.e rpm signature require, unshare network, ...)