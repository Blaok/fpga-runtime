#!/bin/bash
set -e

function install-frt-for-ubuntu() {
  local codename="$1"
  sudo tee /etc/apt/sources.list.d/frt.list <<EOF
deb [arch=amd64] https://about.blaok.me/fpga-runtime ${codename} main
EOF
  wget -O - https://about.blaok.me/fpga-runtime/frt.gpg.key | sudo apt-key add -

  sudo apt-get update
  sudo apt-get install -y libfrt-dev
}

function install-frt-for-centos() {
  local version="$1"
  sudo yum install -y "https://github.com/Blaok/fpga-runtime/releases/latest/download/frt-devel.centos.${version}.x86_64.rpm"
}

source /etc/os-release

case "${ID}.${VERSION_ID}" in
ubuntu.18.04 | ubuntu.20.04)
  install-frt-for-ubuntu "${UBUNTU_CODENAME}"
  ;;
centos.7)
  install-frt-for-centos "${VERSION_ID}"
  ;;
*)
  echo "unsupported os" >&2
  exit 1
  ;;
esac
