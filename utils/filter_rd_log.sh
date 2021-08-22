#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>
# Use: we only care about read latency

DEFLOG=$1
DIR=$(dirname $DEFLOG)
F=$(basename $DEFLOG)
RDLOG=${F%.*}-rd_lat.log

echo $RDLOG

# In replay.c, 1 means read and 0 means write [[opposite of FIO]]
awk -F, '{if ($3 == 1) print $0}' $DEFLOG > $DIR/$RDLOG
