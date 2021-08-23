#!/bin/bash
#--------------------------------------------------------------

# Change the system benchmark mode: IODA, Base, etc.

if [[ $# != 1 ]]; then
    echo ""
    echo "  Usage: (set RAID+FEMU into the following modes)"
    echo ""
    echo "     - $(basename $0) [ioda | iod1 | iod2 | iod3 | base | ideal]"
    echo ""
    exit
fi

mode=$1

function reset_diskstats()
{
    echo ""
    echo "==> Reset diskstats"
    echo ""
    # reset diskstats "/proc/diskstats", /sys/block/*/stat
    echo 0 | sudo tee /sys/block/md0/stat
    for i in 0 1 2 3; do
        echo 0 | sudo tee /sys/block/nvme${i}n1/stat
    done
}

# Enter IODA mode
if [[ $mode == "ioda" ]]; then
    echo ""
    echo "===> Setting up IODA mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 2
    flip 1
    flip 3
    flip 5
    flip 7
    flip 8
    flip 13
    flip 25
    } >/dev/null 2>&1
    echo "===> In IODA mode now ... [done]"

elif [[ $mode == "iod3" ]]; then
    echo ""
    echo "===> Setting up IOD3 (TW-only) mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 3
    flip 1
    flip 3
    flip 5
    flip 7
    flip 8
    flip 13
    flip 24
    } >/dev/null 2>&1
    echo "===> In IOD3 mode now ... [done]"

elif [[ $mode == "iod2" ]]; then  # IOD1 (GCT), only proactive reconstruction under fast-fail
    echo ""
    echo "===> Setting up IOD2 (BRT) mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 2
    flip 1
    flip 3
    flip 5
    flip 7
    flip 9
    flip 25
    } >/dev/null 2>&1
    echo "===> In IOD2 mode now ... [done]"

elif [[ $mode == "iod1" ]]; then # IOD1 (Random under >=2GCs)
    echo ""
    echo "===> Setting up IOD1 (Random under >=2GCs) mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 1
    flip 1
    flip 3
    flip 5
    flip 7
    flip 9
    flip 25
    } >/dev/null 2>&1
    echo "===> In IOD1 mode now ... [done]"

# Enter Base mode
elif [[ $mode == "base" ]]; then

    echo ""
    echo "===> Setting up Base mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 0
    flip 1
    flip 3
    flip 5
    flip 7
    flip 9
    flip 25
    } >/dev/null 2>&1
    echo "===> In Base mode now ... [done]"

# NoGC/Ideal mode
elif [[ $mode == "ideal" ]]; then

    echo ""
    echo "===> Setting up Ideal (NoGC) mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 0
    flip 2
    flip 3
    flip 5
    flip 7
    flip 9
    flip 25
    } >/dev/null 2>&1
    echo "===> In Ideal (NoGC) mode now ... [done]"


elif [[ $mode == "nossd" ]]; then

    echo ""
    echo "===> Setting up NoSSD mode ..."
    {
    getcnt
    rstcnt
    changeReadPolicy 0
    flip 2
    flip 4
    flip 5
    flip 7
    flip 9
    flip 25
    } >/dev/null 2>&1
    echo "===> In NoSSD mode now ... [done]"

else
    echo ""
    echo "===> Unsupported mode: $mode"
    echo ""
    exit
fi

reset_diskstats >/dev/null 2>&1

