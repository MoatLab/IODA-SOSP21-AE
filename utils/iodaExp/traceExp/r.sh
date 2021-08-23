#!/bin/bash
#
# Script to run IODA experiments
#

export REPLAYER="replayer/replayer"
export MDDEV="/dev/md0"
export OUTPUTDIR="sosp21-ae-rst"

# The list of workloads we run
export WL="tpcc azurestorage bingindex bingselect cosmos dtrs est lmbe msnfs"

# $1: trace
# $2: mode
run_1trace_1mode() {
    local traceworkload=$1
    local mode=$2

    echo
    echo "**** Running [$traceworkload] in [${mode}] mode ..."
    echo

    # Switch to a certain IODA mode
    to.sh $mode

    outputf=${traceworkload}-${mode}.log
    sudo $REPLAYER $MDDEV traces/${traceworkload} $OUTPUTDIR/$outputf

    # Process the latency log to extract read latencies
    ./replayer/filter_rd_log.sh $OUTPUTDIR/$outputf >/dev/null 2>&1
}

# run "ioda" mode for all workloads (in batch)
# Note: this requires a different version of iodaFEMU when starting the VM
run_bat_ioda() {
    for w in $WL; do
        for m in "ioda"; do
            run_1trace_1mode $w $m
        done
    done
}

# run modes other than "ioda" for all workloads (in batch)
run_bat_nonioda() {
    for w in $WL; do
        for m in "base" "ideal" "iod1" "iod2" "iod3"; do
            run_1trace_1mode $w $m
        done
    done
}

# run "ioda" mode for one workload
# $1: workload, e.g., "tpcc", "lmbe", etc.
run_sgl_ioda() {
    local w=$1

    for m in "ioda"; do
        run_1trace_1mode $w $m
    done
}

# run non-"ioda" mode for one workload [i.e., run "base" "ideal" "iod1" "iod2" "iod3" modes all together]
# $1: workload, e.g., "tpcc", "lmbe", etc.
run_sgl_nonioda() {
    local w=$1
    for m in "base" "ideal" "iod1" "iod2" "iod3"; do
        run_1trace_1mode $w $m
    done
}

help() {
    echo ""
    echo "One could run the workloads in batch or one by one"
    echo "(1). Run multiple workloads in batch: run all 9 workloads one by one automatically in either "ioda" or "non-ioda" mode, here "non-ioda" refers to the aggregated "ideal base iod1 iod2 iod3"."
    echo "    Usage: run_bat_ioda"
    echo "    Usage: run_bat_nonioda"

    echo ""
    echo ""
    echo "(2). Run one workload at a time"
    echo "    Usage:   run_sgl_ioda [workload], e.g., run_sgl_ioda tpcc"
    echo "    Usage:   run_sgl_nonioda [workload], e.g., run_sgl_nonioda tpcc"
    echo ""
}

help
