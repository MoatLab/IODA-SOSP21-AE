#!/bin/bash
# Create a RAID-5 array
# Huaicheng <huaicheng@cs.uchicago.edu>

if [[ -b /dev/md0 ]]; then
    echo "==> WARNING: /dev/md0 already exists! Skip array assembling..."
    exit
fi

sudo mdadm --create /dev/md0 --level=5 -c 4 --raid-devices=4 /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1 --assume-clean

sleep 1
echo 32768 | sudo tee /sys/block/md0/md/stripe_cache_size
echo 0 | sudo tee /sys/block/md0/md/group_thread_cnt
