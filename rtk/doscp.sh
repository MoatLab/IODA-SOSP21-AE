#!/bin/sh

if [ -z $1 ]; then
    echo Error
    exit
fi

filename_base=$2
title=$1

FP=${title}-${filename_base}

echo "Clearing ${FP} ..."
./clear.sh ${FP}
echo "Creating raw/${FP} ..."
mkdir raw/${FP}

num="1 2 3 4 5" 
for i in $num; do
    for label in "nopgc nosync nogc" "nopgc nosync ebusy" "nopgc sync100ms gct" "nopgc sync100ms ktw" "nopgc nosync gct" "nopgc nosync def"; do
        set -- $label
        is_pgc=$1
        is_sync=$2
        exp=$3
        echo "==> Copying ${title}-${exp}-${filename_base}-${is_sync}-${is_pgc} ..."
        rsync -azP -e 'ssh -p10101' "huaicheng@localhost:~/tifa/replayer/${title}-${exp}-${filename_base}-${is_sync}-${is_pgc}-${i}-rd_lat.log" ./raw/${FP}/ 2>/dev/null
        rsync -azP -e 'ssh -p10101' "huaicheng@localhost:~/tifa/replayer/${title}-${exp}-${filename_base}-${is_sync}-${is_pgc}-${i}-wr_lat.log" ./raw/${FP}/ 2>/dev/null
        rsync -azP -e 'ssh -p10101' --exclude='*kern.log' "huaicheng@localhost:~/tifa/replayer/internal-metrics/${title}-${exp}-${filename_base}-${is_sync}-${is_pgc}-${i}" ./internal-metrics/${FP}/ 2>/dev/null
    done
done    

echo "One shot on ${FP} ..."
./all.sh ${FP}

