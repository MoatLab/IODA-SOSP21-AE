#!/bin/bash
# Huaicheng Li <huaicheng@cs.uchicago.edu>

ME=huaicheng
TOPDIR=$HOME/git/oc-qemu
IMGDIR="./images/"
OCIMG=$IMGDIR/nvme.raw
OCKERNEL=images/bzimages/bzImage 
#OCKERNEL=images/bzimages/bzImage-ioda # For IODA
MNT=$TOPDIR/tmpfs
OCPCIID=

function chk_ramdisk_img()
{
    is_mounted=$(mount | grep "$MNT")

    if [[ "$is_mounted" == "" ]]; then
        mkdir -p $MNT
        sudo mount -t tmpfs -o size=2G tmpfs $MNT
    else
        # always create a new image file upon start
        rm -rf $MNT/*
    fi

    for i in $(seq 1); do
        [[ ! -e $MNT/test${i}.raw ]] && ./qemu-img create -f raw $MNT/test${i}.raw 2G
    done
}

function chk_local_img()
{
    # always create a new image file
    if [[ -e $OCIMG ]]; then
        rm -f $OCIMG
    fi
    ./qemu-img create -f raw $OCIMG 1G
}

function dev_assign()
{
    local PCIID=$(lspci | grep -i "CNEX" | awk '{print $1}')
    if [[ "$PCIID""X" == "X" ]]; then
        echo "No CNEX OpenChannel Device found!"
        exit
    fi

    sudo modprobe pci_stub

    # for now, only support one OC device installed
    local cnt=0
    for i in $(lspci | grep -i "CNEX" | awk '{print $1}'); do
        ((cnt++))
        if [[ $cnt -gt 1 ]]; then
            echo "You have more than 1 OpenChannel SSDs!"
            exit
        fi
    done

    DEVID=$(lspci -n | grep $PCIID | awk '{print $3}' | tr ":" " ")
    PCIIDF="0000:""$PCIID"

    echo "$DEVID" | sudo tee /sys/bus/pci/drivers/pci-stub/new_id
    echo "$PCIIDF" | sudo tee /sys/bus/pci/devices/$PCIIDF/driver/unbind
    sleep 2
    echo "$PCIIDF" | sudo tee /sys/bus/pci/drivers/pci-stub/bind
    echo "$DEVID" | sudo tee /sys/bus/pci/drivers/pci-stub/remove_id

    OCPCIID=$PCIID
}

sudo iodaFEMU/build-femu/x86_64-softmmu/qemu-system-x86_64 \
    -name "tifaVM" \
    -cpu host \
    -smp 24 \
    -m 16G \
    -enable-kvm \
    -kernel "${OCKERNEL}" \
    -append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr log_buf_len=128M loglevel=4" \
    -boot menu=on \
    -drive file=$IMGDIR/ioda.qcow2,if=ide,cache=none,aio=native,format=qcow2 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -netdev user,id=user0,hostfwd=tcp::10101-:22 \
    -device virtio-net-pci,netdev=user0 \
    -nographic | tee ./femu.log 2>&1 \
