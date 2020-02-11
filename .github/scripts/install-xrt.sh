#!/bin/bash
set -e
version="$(grep --perl --only '(?<=VERSION_ID=").+(?=")' /etc/os-release)"
file="xrt_201920.2.3.1301_${version}-xrt.deb"
curl "https://www.xilinx.com/bin/public/openDownload?filename=${file}" \
  --location --output "${file}"
sudo apt-get update
sudo apt-get install -y python-pyopencl
sudo apt-get install -f "./${file}"
XILINX_XRT=/opt/xilinx/xrt
echo "::set-env name=XILINX_XRT::${XILINX_XRT}"
echo "::set-env name=LD_LIBRARY_PATH::${XILINX_XRT}/lib"
echo "::add-path::${XILINX_XRT}/bin"
