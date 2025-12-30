FROM gcc@sha256:36d341e2a0db3d8036872d62bd6f5a80fa7b0633d53586c6150fbd9428166b6a

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

