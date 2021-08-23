#!/bin/bash
# Create a RAID-5 array
# Huaicheng <huaicheng@cs.uchicago.edu>

sudo mdadm -S /dev/md0
for i in $(seq 0 3); do
    sudo mdadm /dev/md0 -r nvme${i}n1
done

sudo modprobe -r md
