#!/bin/bash
set -e

codename="$(grep --perl --only '(?<=UBUNTU_CODENAME=).+' /etc/os-release)"

sudo tee /etc/apt/sources.list.d/frt.list <<EOF
deb [arch=amd64] https://about.blaok.me/fpga-runtime ${codename} main
EOF
wget -O - https://about.blaok.me/fpga-runtime/frt.gpg.key | sudo apt-key add -

sudo apt update
sudo apt install -y libfrt-dev
