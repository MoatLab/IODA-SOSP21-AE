#!/bin/bash
#
# This script builds iodaFEMU and iodaLinux
# Usage: ./build-ioda.sh
#
# Note: Please cd into IODA-SOSP21-AE/ first, and then run "./build-ioda.sh"
#

# First, install IODA dependencies
echo ""
echo "====> [1] Installing IODA dependencies ..."
echo ""
sudo ./utils/ioda-pkgdep.sh >/dev/null 2>&1


# Second, build iodaFEMU
echo ""
echo "====> [2] Building iodaFEMU ..."
echo ""
cd src/iodaFEMU
mkdir -p build-femu
cd build-femu
./femu-compile.sh >/dev/null 2>&1

# Third, build iodaLinux
echo ""
echo "====> [3] Building iodaLinux ..."
echo ""
cd ../../
cd iodaLinux
cp ioda-config .config
make oldconfig >/dev/null 2>&1
make -j32 >/dev/null 2>&1

echo ""
echo "====> Done with building IODA!"
echo "Please check the compiled iodaFEMU at [src/iodaFEMU/build-femu/x86_64-softmmu/qemu-system-x86_64] and iodaLinux rootfs aat [src/iodaLinux/arch/x86/boot/bzImage]"
echo ""

