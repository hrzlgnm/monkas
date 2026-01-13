FROM gcc@sha256:bc5daa3a6407ceb0b0928bbeb0f552d134ac88bb8e88f420e61e28dca72566f8

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

