FROM gcc@sha256:70eaceecb9377ff4b87ea08486b04787694455c5cdee9f1ef19ba74f350c0757

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    clang \
    clang-format \
    cmake \
    jq \
    libasan8 \
    libclang-rt-dev \
    libgflags-dev \
    libmnl-dev \
    libspdlog-dev \
    libubsan1 \
    ninja-build \
    python3-pip \
    && python3 -m pip install --break-system-packages --no-cache-dir gersemi==0.22.1 \
    && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.source="https://github.com/hrzlgnm/monkas"
LABEL org.opencontainers.image.title="monkas GitHub GCC"
LABEL org.opencontainers.image.base.name="docker.io/library/gcc"

