#!/bin/bash
set -e
version="$(grep --perl --only '(?<=VERSION_ID=").+(?=")' /etc/os-release)"
if test "${version//.*/}" -le 22; then
  file="xrt_202220.2.14.354_${version}-amd64-xrt.deb"
  curl "https://www.xilinx.com/bin/public/openDownload?filename=${file}" \
    --location --output "${file}"
  sudo apt-get update
  sudo apt-get install -y -f "./${file}"
else
  sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libcurl4-openssl-dev \
    libdrm-dev \
    libncurses-dev \
    libprotobuf-dev \
    libssl-dev \
    libudev-dev \
    libxml2-dev \
    libyaml-dev \
    lsb-release \
    ocl-icd-dev \
    ocl-icd-opencl-dev \
    pkg-config \
    protobuf-compiler \
    uuid-dev \

  git clone https://github.com/Xilinx/XRT.git
  XRT/build/build.sh -opt -noctest
  sudo apt-get install -y -f ./XRT/build/Release/xrt*-amd64-xrt.deb
fi

XILINX_XRT=/opt/xilinx/xrt
echo "XILINX_XRT=${XILINX_XRT}" >>$GITHUB_ENV
echo "CPATH=${XILINX_XRT}/include" >>$GITHUB_ENV
echo "LD_LIBRARY_PATH=${XILINX_XRT}/lib" >>$GITHUB_ENV
echo "${XILINX_XRT}/bin" >>$GITHUB_PATH
