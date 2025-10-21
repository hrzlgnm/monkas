FROM gcc@sha256:ff774ba437fa734572d945ceb9d355b7257500e6d306e9a00ef5b51109aebc52

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
    && python3 -m pip install --break-system-packages --no-cache-dir gersemi==0.22.3 \
    && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.source="https://github.com/hrzlgnm/monkas"
LABEL org.opencontainers.image.title="monkas GitHub GCC"
LABEL org.opencontainers.image.base.name="docker.io/library/gcc"

