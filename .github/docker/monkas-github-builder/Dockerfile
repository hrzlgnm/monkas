FROM gcc@sha256:0ce0b38744ff10282b446265b4ded84252a62657abad9d1b2a0e7a1648ea2317

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    clang \
    cmake \
    jq \
    libasan8 \
    libclang-rt-dev \
    libgflags-dev \
    libmnl-dev \
    libspdlog-dev \
    libubsan1 \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.source="https://github.com/hrzlgnm/monkas"
LABEL org.opencontainers.image.title="monkas GitHub Builder"
LABEL org.opencontainers.image.base.name="docker.io/library/gcc"

