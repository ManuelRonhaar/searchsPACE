#!/usr/bin/env bash
set -euxo pipefail

apt-get update

DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    unzip \
    pkg-config \
    ca-certificates

update-ca-certificates

# Install OR-Tools
cd /tmp
ORTOOLS_FILE=or-tools_amd64_debian-12_cpp_v9.12.4544.tar.gz
wget https://github.com/google/or-tools/releases/download/v9.12/${ORTOOLS_FILE}

mkdir -p /opt/ortools

tar -xf ${ORTOOLS_FILE} \
    --strip-components=1 \
    -C /opt/ortools

echo "export CMAKE_PREFIX_PATH=/opt/ortools:\$CMAKE_PREFIX_PATH" >/etc/profile.d/ortools.sh
echo "export LD_LIBRARY_PATH=/opt/ortools/lib:\$LD_LIBRARY_PATH" >>/etc/profile.d/ortools.sh