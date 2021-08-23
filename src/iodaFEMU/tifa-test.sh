#!/bin/bash
# Huaicheng Li <huaicheng@cs.uchicago.edu>

ME=huaicheng
TOPDIR=$HOME/git/oc-qemu
IMGDIR=/proj/ucare/ronald/u7_dir
OCIMG=$IMGDIR/nvme.raw
OCKERNEL=/proj/ucare/ronald/git/tifaLinux/arch/x86_64/boot/bzImage
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

#dev_assign

# Create another event loop (for dataplane)
# -object iothread,id=iothread0 \
# -device virtio-blk-pci,iothread=iothread0,drive=id0 \

# Add a NVMe drive
# -drive file=<file>,if=none,id=<drive_id>
# -device nvme,drive=<drive_id>,serial=<serial>

# Add OC attributes to NVMe drives
#
#

# Add a virtio-blk drive
# -device virtio-blk-pci,drive=id1,ioeventfd=on,request-merging=off \
# -drive file=$IMGDIR/hdd.raw,if=none,cache=none,aio=native,format=raw,id=id2 \

# Add a SATA drive
# -device ich9-ahci,id=ahci \
# -device ide-drive,drive=id2,bus=ahci.0 \

# Obsolete way to create a virtio network card
# -net nic,model=virtio \
# -net user,hostfwd=tcp::8080-:22 \

#chk_local_img

#qemu-system-x86_64 -m 4G -smp 1 --enable-kvm
#-hda $LINUXVMFILE -append "root=/dev/sda1"
#-kernel "/home/foobar/git/linux/arch/x86_64/boot/bzImage"
#-drive file=blknvme,if=none,id=mynvme
#-device nvme,drive=mynvme,serial=deadbeef,namespaces=1,lver=1,nlbaf=5,lba_index=3,mdts=10
#-virtfs local,path=/home/huaicheng/share/,security_model=passthrough,mount_tag=host_share \

    #-kernel "${OCKERNEL}" \
    #-append "root=/dev/sda" \
    #-s -S \
    #-drive file=$OCIMG,if=none,aio=threads,format=raw,id=mynvme \
    #-device nvme,drive=mynvme,serial=deadbeef,namespaces=1,lver=1,ll2pmode=0,nlbaf=5,lba_index=3,mdts=10,ldebug=1,lnum_ch=1,lnum_lun=2,lnum_pln=1,lpgs_per_blk=256,lsec_size=4096 \
    # 0d;00.0 is my OC device!!!
    #-device pci-assign,host=0d:00.0 \
    #-device pci-assign,host="$OCPCIID" \
    #-kernel "${OCKERNEL}" \
    #-append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0" \
    # OLD usage: -device pci-assign,host=0a:00.0 \


    #-drive file=nvme01.img,if=none,aio=threads,format=raw,id=nvme01 \
    #-device nvme,drive=nvme01,serial=nvme01 \
    #-drive file=nvme02.img,if=none,aio=threads,format=raw,id=nvme02 \
    #-device nvme,drive=nvme02,serial=nvme02 \
    #-drive file=nvme03.img,if=none,aio=threads,format=raw,id=nvme03 \
    #-device nvme,drive=nvme03,serial=nvme03 \
    #-drive file=nvme04.img,if=none,aio=threads,format=raw,id=nvme04 \
    #-device nvme,drive=nvme04,serial=nvme04 \




    #-kernel "${OCKERNEL}" \
    #-append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr" \


sudo x86_64-softmmu/qemu-system-x86_64 \
    -name "tifaVM" \
    -cpu host \
    -smp 16 \
    -m 16G \
    -enable-kvm \
    -kernel "${OCKERNEL}" \
    -append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr log_buf_len=128M loglevel=4" \
    -boot menu=on \
    -drive file=$IMGDIR/u14s.qcow2,if=ide,cache=none,aio=native,format=qcow2 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -netdev user,id=user0,hostfwd=tcp::10101-:22 \
    -device virtio-net-pci,netdev=user0 \
    -nographic | tee log 2>&1 \

#-s -S

