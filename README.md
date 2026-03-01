# IPFS racing multi-gateway integration for Chromium

## Overview

See a [blog post](https://blog.ipfs.tech/2023-05-multigateway-chromium-client/) about it. 

The intended audience there was genpop & the IPFS-aware. If, however, you're more of a browser person, the [explainer](doc/explainer.md) might make more sense to you. Maybe.

Also, slides for a BlinkOn [lightning talk](doc/slides/blinkon23.md).

### Basic demo
[demo.webm](https://github.com/little-bear-labs/ipfs-chromium/assets/97759690/ae072a58-f5de-4270-8d48-2c858d9b17b1)

## Documentation
For info on how to use the library outside of Chromium, see [its readme](https://gitlab.com/jbt/ipfs_client/-/blob/main/README.md).
See also the [Doxygen](https://little-bear-labs.github.io/ipfs-chromium/annotated.html) docs.

## Design

See [doc/original_design.md](doc/original_design.md) for original intent, and a reasonable overview of the big picture.

See [doc/design_notes.md](doc/design_notes.md) for more detailed notes on some of the more critical features, as-implemented.

## Tor Onion Service

The `.xyz` domain interceptor can route traffic through a Tor onion service
backed by [maceip/Tor](https://github.com/maceip/Tor) (included as a
submodule in `third_party/tor`).

The `TorOnionService` class manages the full lifecycle: writing a `torrc`,
launching the `tor` binary, reading back the generated `.onion` hostname,
and exposing a local SOCKS5 proxy port for proxying outbound connections.

To build with Tor from source:

```
cmake -DTOR_SOURCE_DIR=third_party/tor ..
```

Or point to a pre-built tor binary:

```
cmake -DTOR_BINARY=/usr/bin/tor ..
```

## Building

See [doc/building.md](doc/building.md)
