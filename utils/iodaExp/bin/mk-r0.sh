#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>

sudo mdadm --create /dev/md0 --level=0 -c 4 --raid-devices=2 /dev/nvme0n1 /dev/nvme1n1 --assume-clean
