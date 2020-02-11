#!/bin/bash
set -e
version="$(grep --perl --only '(?<=VERSION_ID=").+(?=")' /etc/os-release)"
file="xrt_201920.2.3.1301_${version}-xrt.deb"
curl "https://www.xilinx.com/bin/public/openDownload?filename=${file}" \
  --location --output "${file}"
sudo apt-get update
sudo apt-get install -y python-pyopencl
sudo apt-get install -f "./${file}"
cat <<EOF |
export XILINX_XRT=/opt/xilinx/xrt
export LD_LIBRARY_PATH=$XILINX_XRT/lib:$LD_LIBRARY_PATH
export PATH=$XILINX_XRT/bin:$PATH
EOF
  sudo tee /etc/profile.d/xrt.sh >/dev/null
