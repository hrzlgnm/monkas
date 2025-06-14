FROM gcc@sha256:a4da41bd17f92a512d2e59f3272ae6315fd01ff4857875f8b6c7dac207410ead

RUN apt-get update --yes && \
    apt-get install -yes --no-install-recommends \
    clang-format \
    cmake \
    libasan6 \
    libgflags-dev \
    libmnl-dev \
    libspdlog-dev \
    libubsan1 \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.source="https://github.com/hrzlgnm/monkaS"
LABEL org.opencontainers.image.title="monkaS GitHub GCC"
LABEL org.opencontainers.image.base.name="docker.io/library/gcc"

