FROM ubuntu:24.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libssl-dev \
    libnghttp2-dev \
    zlib1g-dev \
    wget \
    unzip \
    pkg-config \
    git \
    libasound2-dev \
    libpulse-dev \
    libx11-dev \
    libxext-dev \
    libwayland-dev \
    rsync

# Create working directory
WORKDIR /drod

CMD ["/bin/bash"]