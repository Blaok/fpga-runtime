#!/bin/bash
set -e
version="$(grep --perl --only '(?<=VERSION_ID=").+(?=")' /etc/os-release)"
if test "${version//.*/}" -le 18; then
  file="xrt_202010.2.6.655_${version}-amd64-xrt.deb"
  curl "https://www.xilinx.com/bin/public/openDownload?filename=${file}" \
    --location --output "${file}"
  sudo apt-get update
  sudo apt-get install -y python-pyopencl
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
echo "::set-env name=XILINX_XRT::${XILINX_XRT}"
echo "::set-env name=LD_LIBRARY_PATH::${XILINX_XRT}/lib"
echo "::add-path::${XILINX_XRT}/bin"
