# red-pak: an Ultra Light Weight Container for embedded applications

![redpak log](images/logo_redpak.png "redpak logo")

## Links

- github: https://github.com/redpesk-labs/red-pak

## Red-pak motivations

- Provide application isolation
    – Restricted filesystem visibility
    - Resources access/usage (API, CPU, RAM, Network, …)
    - Built-in security model with MAC (Mandatory Access Control)
- Maximize resource sharing & minimize system overload
    - No duplication of root-fs
    - Reuse shared libraries between instances
    - Restrict RAM, Disk, CPU containerization cost
    - Boost container startup time
- Prevent “diplomatic suitcase” container model
    - Strict enforcement on installed packages & dependencies
    - Keep the system auditable
    - White box container model

Everyone understands that installing a software component on millions of cars,
on a submarine or in a train is very different from installing a new application on a desktop or a phone.

red-pak targets embedded devices used within critical infrastructure (automotive, boat, submarine, train, civil infrastructure, medical, ...).
red-pak does not use black box containers, on the contrary it enforces a white box model where the global coherency of the system can be proven.

While each node of a red-pak family owns an atomic subset of the full rootfs tree, the global coherency of each node is statically verified. Installation/updates can be proven before installation on the embedded target by an adequate CI/Build-system such as Redpesk (http://docs.redpesk.bzh).
