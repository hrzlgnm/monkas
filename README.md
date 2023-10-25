# monkas

Netlink Socket experiments using libmnl

# Denpendencies

- [cmake](https://cmake.org/)
- [libmnl](https://netfilter.org/projects/libmnl/)
- [spdlog](https://github.com/gabime/spdlog)
- [gflags](https://github.com/gflags/gflags)

## Quick install for debian/ubuntu based distors

```console
sudo apt install build-essential cmake libmnl-dev libspdlog-dev libgflags-dev
```

# Quick Start

```console
cmake -DBUILD_EXAMPLES=ON -B build -S .
cmake --build build
build/examples/cli/monkas
```
