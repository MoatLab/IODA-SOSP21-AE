#!/bin/bash

TDIR=traces
BIN=/home/huaicheng/tifa/bin

# tpcc-6-0ed-ms msn-fs-60-0ed-ms

#for i in c,3 e,5; do IFS=","; set -- $i; echo $1 and $2; done
for tf in "lmbe-iosize-4-rerate-4"; do
    OLDIFS=$IFS; IFS=',';
    for i in nogc,nosync; do
        set -- $i
        echo $1 and $2
        exp=$1
        label=$2
        ./tifa reset
        echo ""
        echo "*******Start running $tf-$exp at $(date)********"
        echo ""
        if [[ $exp == "nogc" ]]; then
            flip 2 # disable GC delay
        else
            flip 1 # enable GC delay
        fi

        if [[ $exp == "gct" ]]; then
            changeReadPolicy 2
            flip 10
        else
            changeReadPolicy 0
            flip 11
        fi

        flip 7

        if [[ $label == "nosync" ]]; then
            flip 9
        else
            flip 8
        fi

        case $label in
            "sync1s")
                echo synch1s
                flip 12
                ;;
            "sync100ms")
                echo sync100ms
                flip 13
                ;;
            "sync2s")
                echo sync2s
                flip 14
                ;;
            "sync10ms")
                echo sync10ms
                flip 15
                ;;
            "sync40ms")
                echo sync40ms
                flip 16
                ;;
            "sync200ms")
                echo sync200ms
                flip 17
                ;;
            "sync400ms")
                echo sync400ms
                flip 18
                ;;
            "nosync")
                echo Hopefully nosync
                ;;
            *)
                echo Not recognized label: $label
                ;;
        esac
        # experiment profile name
        FP=$label-$tf-${exp}

        sudo ./replayer /dev/md0 $TDIR/$tf ${FP}.log
        echo ""
        echo "*******End running $tf-$exp at $(date)*********"
        echo ""
        ./filter_rd_log.sh ${FP}.log

        ./tifa ${FP}

        sleep 1
    done
    IFS=$OLDIFS
done
