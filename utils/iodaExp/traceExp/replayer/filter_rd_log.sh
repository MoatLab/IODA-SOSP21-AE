#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>
# Use: we only care about read latency

DEFLOG=$1
DIR=$(dirname $DEFLOG)
F=$(basename $DEFLOG)
RDLOG=${F%.*}-rd_lat.log


if [[ $# == 1 ]]; then
    echo $RDLOG
    # In replay.c, 1 means read and 0 means write [[opposite of FIO]]
    #awk -F, '{if ($3 == 1 && $2 < 100000) print $0}' $DEFLOG > $DIR/$RDLOG
    awk -F, '{if ($3 == 1) print $0}' $DEFLOG > $DIR/$RDLOG
    exit
fi

# Otherwise, get write latency log
WRLOG=${F%.*}-wr_lat.log
echo $WRLOG
awk -F, '{if ($3 == 0) print $0}' $DEFLOG > $DIR/$WRLOG
