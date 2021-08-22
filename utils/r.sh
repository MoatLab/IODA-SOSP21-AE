#!/bin/bash

LOGDIR=logs
TDIR=traces/
BIN=/home/huaicheng/tifa/bin
TITLE="sosp21"

if [ -n "$1" ]; then
    tfs=$1
    nums="1"
else
    tfs="tpcc"
    nums="1" 
fi

if [ -n "$2" ]; then
    seq=-$2
else
    seq=""
fi
if [ ! -e "/dev/md0" ]; then
    cd ~/tifa ; ./ss ; cd replayer
fi

for tf in $tfs; do
for num in $nums; do
    for i in "nopgc nosync def" "nopgc sync100ms gct"; do #"nopgc nosync gct" "nopgc sync100ms gct" "nopgc nosync def" "nopgc nosync nogc" "nopgc nosync ebusy"; do 
        set -- $i
        echo $i
        pgc_label=$1
        sync_label=$2
        exp=$3
            #cd ~/tifa ; ./ss ; cd replayer
            retval=1
            while [ $retval -eq 1 ]; do
                sudo ./tifa reset 
                echo ""
                echo "*******Start running $tf-$exp-$num at $(date)********"
                echo ""
                
                if [[ $exp == "nogc" ]]; then
                    flip 2 &>/dev/null # disable GC delay
                else
                    flip 1 &>/dev/null # enable GC delay
                fi

                flip 11 &>/dev/null # Disable log free blocks 
                
                if [[ $exp == "gct" ]]; then
                    ${BIN}/changeReadPolicy 2
                elif [[ $exp == "ebusy" ]]; then
                    ${BIN}/changeReadPolicy 1
                elif [[ $exp == "ktw" ]]; then
                    ${BIN}/changeReadPolicy 3
                elif [[ $exp == "proactive" ]]; then
                    ${BIN}/changeReadPolicy 4
                else
                    ${BIN}/changeReadPolicy 0
                fi
                # Check changeReadPolicy
                sudo tail -n 1 /var/log/kern.log

                flip 7 &>/dev/null

                # Enable/Disable sync

                if [[ $sync_label == "nosync" ]]; then
                    flip 9 &>/dev/null
                else 
                    flip 8 &>/dev/null
                fi

                # Enable/Disable Preemptive GC
                if [[ $pgc_label == "pgc" ]]; then
                    # Enable preemptive
                    flip 24 &>/dev/null
                else
                    # Disable preemptive
                    flip 25 &>/dev/null
                fi

                # experiment profile name
                FP=$TITLE-$exp-$tf-$sync_label-$pgc_label-$num

                stdbuf -o0 sudo ./replayer/replayer /dev/md0 $TDIR/$tf ${FP}.log | awk '1;{fflush()}' RS='\r' | ./detect_late.sh $$ $FP
                #sudo ./replayer/replayer /dev/md0 $TDIR/$tf ${FP}.log
                retval=$?
                echo ""
                echo "*******End running $tf-$exp at $(date)*********"
                echo ""
                
                echo ""
                echo "Filtering ${FP} ..."
                echo ""
                ./filter_rd_log.sh ${FP}.log
                ./tifa ${FP}
                sleep 1
            done
        done
    done
done
