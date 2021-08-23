#!/bin/bash
#
# This script builds FEMU and Linux for IODA
# Usage: ./build.sh
#
# Note: Please cd into IODA-SOSP21-AE/ first, and then run "./build.sh"
#

IODA_TOPDIR=$(pwd)

red=`tput setaf 1`
green=`tput setaf 2`
blue=`tput setaf 4`
reset=`tput sgr0`

IODA_BUILD_LOG="ioda-build.log"

sudo swapoff -a

echo -e "\n====> Start building IODA ... should take ${green}<10min${reset} to finish with 32 cores on the server\n"

# First, install IODA dependencies
echo ""
echo "====> ${green}[1/3]${reset} Installing IODA dependencies ..."
echo ""
sudo ./utils/ioda-pkgdep.sh >${IODA_BUILD_LOG} 2>&1


# Second, build iodaFEMU
echo ""
echo "====> ${green}[2/3]${reset} Building iodaFEMU ..."
echo ""
cd ${IODA_TOPDIR}/src/iodaFEMU
mkdir -p build-femu
cd build-femu
make clean >/dev/null 2>&1
./femu-compile.sh >>${IODA_BUILD_LOG} 2>&1

# Third, build iodaLinux
echo ""
echo "====> ${green}[3/3]${reset} Building iodaLinux ..."
echo "      Note: this is an optional step since we will use a pre-compiled version of the Linux rootfs in our VM image"
echo ""
cd ${IODA_TOPDIR}/src/iodaLinux
make clean >/dev/null 2>&1
cp ioda-config .config
make oldconfig >>${IODA_BUILD_LOG} 2>&1
make -j32 >>${IODA_BUILD_LOG} 2>&1
cd ../../


IODA_FEMU_BIN="src/iodaFEMU/build-femu/x86_64-softmmu/qemu-system-x86_64"
NONIODA_FEMU_BIN="src/noniodaFEMU/build-femu/x86_64-softmmu/qemu-system-x86_64"
IODA_LINUX_BIN="src/iodaLinux/arch/x86/boot/bzImage"

if [[ -e ${IODA_FEMU_BIN} && -e ${IODA_LINUX_BIN} ]]; then
    echo ""
    echo "===> ${green}Congrats${reset}, IODA is successfully built!"
    echo ""
    echo "Please check the compiled binaries:"
    echo "  - iodaFEMU at [${blue}${IODA_FEMU_BIN}${reset}] (only for running "ioda" mode experiments)"
    echo "  - noniodaFEMU at [${blue}${NONIODA_FEMU_BIN}${reset}] (for running "base ideal iod1 iod2 iod3" mode experiments)"
    echo "  - iodaLinux rootfs at [${blue}${IODA_LINUX_BIN}${reset}]"
    echo ""

    #cp ${IODA_FEMU_BIN} bin/iodaFemuBin/
    #cp ${IODA_LINUX_BIN} bin/iodaLinuxBin/

else

    echo ""
    echo "===> ${red}ERROR:${reset} Failed to build IODA, please check [${IODA_BUILD_LOG}] and talk to the IODA authors on hotcrp."
    echo ""

fi

