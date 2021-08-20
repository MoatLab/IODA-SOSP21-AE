#!/bin/bash
# Author: Huaicheng <huaicheng@cs.uchicago.edu>
# generate a template gnuplot file 
#

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
    TOPDIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$TOPDIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

TOPDIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )/.."
RAWDIR=$TOPDIR/raw
DATDIR=$TOPDIR/dat
SCRIPTDIR=$TOPDIR/script
PLOTDIR=$TOPDIR/plot
EPSDIR=$TOPDIR/eps
STATDIR=$TOPDIR/stat

################################################################################

function pr_title()
{
    printf "%s,%s,%s,%s,%s,%s\n" "Filename" "Min" "Average" "Median" "Max" "Stddev"
}

INPUT=$1

if [[ $# != 1 ]]; then
    echo ""
    echo "Usage: ./getstat.sh [directory|file]"
    echo ""
    exit
fi

#if [[ ! -f $INPUT || ! -d $RAWDIR/$INPUT ]]; then
    #echo "Error: $INPUT not found!\n"
    #exit
#fi


if [[ -f $INPUT ]]; then

    pr_title
    awk -vFNAME=$(basename $INPUT) -f $SCRIPTDIR/stat.awk $INPUT

elif [[ -d $RAWDIR/$INPUT ]]; then

    if [[ ! -d $STATDIR/$INPUT ]]; then
        mkdir -p $STATDIR/$INPUT
    fi

    STATF=$STATDIR/$INPUT/$INPUT-stat.csv
    if [[ -e $STATF ]]; then
        exit
    fi

    {
        pr_title 
        for i in $RAWDIR/$INPUT/*.tmp; do
            FNAME=$(basename $i)
            awk -vFNAME=$FNAME -f $SCRIPTDIR/stat.awk $i
        done
    } > $STATF

    # show result to terminal
    cat $STATF
fi

