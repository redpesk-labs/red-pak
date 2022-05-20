# red-pak: an Ultra Light Weight Container for embedded applications

![redpak log](docs/images/logo_redpak.png "redpak logo")

## General presentation
Red-pak targets embedded and critical infrastructures:
   - it maximizes resource sharing (no rootfs/sharedlib duplication)
   - it is designed to be auditable. While each individual node is independent, the global coherency is provided by `dnf`/`rpm` and `libsolv` at the core OS level). The red-pak coherency can be statically proven at the CI level before pushing the image to the target.
   - it simplifies container inspection (a node is an atomic subset of a rootfs)
   - it uses standard management tools (`dnf`+`rpm`)
   - built with long term support and cybersecurity in mind.

## Documentation

Documentation is available on redpesk docs website: [https://docs.redpesk.bzh/docs/en/master/redpesk-os/#redpak](https://docs.redpesk.bzh/docs/en/master/redpesk-os/#redpak)
