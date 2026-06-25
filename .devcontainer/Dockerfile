FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    ca-certificates \
    ccache \
    clang \
    clang-format \
    clang-tidy \
    cmake \
    gdb \
    git \
    make \
    pkg-config \
  && rm -rf /var/lib/apt/lists/*
