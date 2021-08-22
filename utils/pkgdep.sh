#!/bin/bash
# Huaicheng <huaicheng@cs.uchicago.edu>
# Please run this script as root.

SYSTEM=`uname -s`

if [[ -f /etc/debian_version ]]; then
    apt-get update
	# Includes Ubuntu, Debian
    apt-get install -y gcc pkg-config git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
    apt-get install -y libaio-dev python exuberant-ctags

    apt-get install -y libncurses5-dev libelf-dev

	# Additional dependencies
	apt-get install -y libnuma-dev

else
    echo "pkgdep: unsupported system type ($SYSTEM), please install QEMU depencies manually"
	exit 1
fi
