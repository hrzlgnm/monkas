<!--toc:start-->

- [monkas](#monkas)
- [Quick Start](#quick-start)
- [Dependencies](#dependencies)
  - [Quick install](#quick-installation-for-debianubuntu-based-distributions)
  <!--toc:end-->

# monkas

Linux netlink socket experiments using `libmnl`

## Quick Start

```console
cmake -B build -S .
cmake --build build
build/examples/cli/monkas
```

## Dependencies

- [cmake](https://cmake.org/)
- [libmnl](https://netfilter.org/projects/libmnl/)
- [spdlog](https://github.com/gabime/spdlog)
- [fmt](https://fmt.dev)
- [gflags](https://github.com/gflags/gflags)

### Quick installation for Debian/Ubuntu based distributions

```console
sudo apt update && sudo apt install build-essential cmake libmnl-dev libspdlog-dev libgflags-dev libfmt-dev
```
