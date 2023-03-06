# monkas
Netlink Socket experiments using libmnl

# Denpendencies
* [cmake](https://cmake.org/)
* [libnml](https://netfilter.org/projects/libmnl/)
* [spdlog](https://github.com/gabime/spdlog)
* [gflags](https://github.com/gflags/gflags)

## Quick install for debian/ubuntu based distors
```console
sudo apt install build-essential cmake libnml-dev libspdlog-dev
```

# Quick Start
```console
cmake -B build -S .
cmake --build build
build/monkas
```
