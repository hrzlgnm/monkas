FROM gcc@sha256:0cc1665930da5d3052d4af019579f1f35dbdf02598ddadfba3b15365b0507de5

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

