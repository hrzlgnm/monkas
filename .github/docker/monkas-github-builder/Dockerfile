FROM gcc@sha256:1c68ef4ba3730c7e70c343212cd322af0e806e7e8c5c713facf4d5f719e155a3

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

