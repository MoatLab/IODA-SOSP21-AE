#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>

sudo mdadm --create /dev/md0 --level=5 -c 4 --raid-devices=4 /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1 --assume-clean
