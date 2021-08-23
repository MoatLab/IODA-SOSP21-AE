#!/bin/bash
# Run all IODA modes in one run

source /home/huaicheng/iodaExp/ioda-env.sh

#-------------------------------------------------------------------------------
# Parameters to change
#-------------------------------------------------------------------------------

# Trace workload name
W="./tpcc-resize-w16.0-trim-s0-e60"
W="msnfs10"
W="azurestorage-drive2.trace-trim-s27000-e30600-rerate-8.0"
#W="bingselection-drive2.trace-trim-s55800-e57600-rerate-0.5"
#W="bingindex-drive2.trace-trim-s6300-e6600-rerate-2.0-resize-w4.0"
#W="cosmos-drive2.trace-trim-s6300-e8100-rerate-0.5"
#W="lmbe-resize-r0.25-w3.0-trim-s1500-e2100"

# Shortname for the workload for file naming
RDIR="azure"

#
#-------------------------------------------------------------------------------
#
LOGFPRE=${RDIR}
METRIC_DIR="${RDIR}/metrics"
EID=201

mkdir -p ${RDIR}
mkdir -p ${METRIC_DIR}

for et in "base" "ideal" "iod3" "iod1" "iod2" "ioda"; do
    TF="${LOGFPRE}-${et}-${EID}"
    LOGF="${RDIR}/${TF}.log"

    to.sh ${et} # adjust IODA mode
    # Truncate FEMU log
    ssh u10 "echo > /home/huaicheng/git/iodaFEMU/build-ioda/log"

    sudo ./replayer /dev/md0 $W ${LOGF}
    sleep 5

    # Process latency log
    ./filter_rd_log.sh ${LOGF}
    ./filter_rd_log.sh ${LOGF} 1

    # Collect metrics
    getcnt > ${METRIC_DIR}/${TF}.getcnt 2>&1
    gc.sh > ${METRIC_DIR}/${TF}.gc 2>&1
    cat /proc/diskstats > ${METRIC_DIR}/${TF}.diskstats 2>&1
    sudo cp /var/log/kern.log > ${METRIC_DIR}/${TF}.kernlog 2>&1
    scp u10:/home/huaicheng/git/iodaFEMU/build-ioda/log ${METRIC_DIR}/${TF}.femulog 2>&1
done
