#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>

trace=TPCC-6-ms
DEV=/dev/tgt0

if [[ $# == 1 ]]; then
    echo ""
    echo " ==> Writes read locations first .."
    sudo ./writer $DEV $trace
    echo ""
fi

./tos.sh

echo " ==> Replaying the trace .."
sudo ./replayer $DEV $trace

cat /sys/block/$DEV/pblk/tos_tt > $trace-log.raw
