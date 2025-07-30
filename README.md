<!--toc:start-->

- [monkas](#monkas)
- [Dependencies](#dependencies)
  - [Quick installation for debian/ubuntu based distros](#quick-install-for-debianubuntu-based-distors)
- [Quick Start](#quick-start)
<!--toc:end-->

# monkas

Netlink Socket experiments using libmnl

# Dependencies

- [cmake](https://cmake.org/)
- [libmnl](https://netfilter.org/projects/libmnl/)
- [spdlog](https://github.com/gabime/spdlog)
- [fmt](https://fmt.dev)
- [gflags](https://github.com/gflags/gflags)

## Quick install for debian/ubuntu based distros

```console
sudo apt install build-essential cmake libmnl-dev libspdlog-dev libgflags-dev libfmt-dev
```

# Quick Start

```console
cmake -B build -S .
cmake --build build
build/examples/cli/monkas
```
