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

# Create a non-root user with UID:GID 1000:1000 for development use
# This ensures files created in the mounted project directory are owned
# by the host user if it has UID 1000 (common on many Linux systems).
RUN groupadd -g 1000 drod-dev 2>/dev/null || true \
 && useradd -m -u 1000 -g 1000 -s /bin/bash drod-dev 2>/dev/null || true \
 && mkdir -p /drod /home/drod-dev \
 && chown -R drod-dev:drod-dev /drod /home/drod-dev

# Set the home directory and default user for interactive shells
ENV HOME=/home/drod-dev
USER drod-dev

# Default command (can override)
CMD ["/bin/bash"]