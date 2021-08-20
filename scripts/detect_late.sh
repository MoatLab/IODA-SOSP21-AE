#!/bin/bash

late_rate_thresh=1

#stdbuf -o0 ./r.sh | awk '1;{fflush()}' RS='\r' | ./detect_late.sh'
while read line; do
    echo $line
    late_perc=$(echo $line | grep 'Late rate:' | cut -d' ' -f 6)
    #late_perc=$(echo $line | grep 'Slack rate:' | cut -d' ' -f 10)
    late_perc=${late_perc%?}
    #late_val=${late_perc:0:1}
    late_val=${late_perc}
    progress=$(echo $line | grep 'Progress:' | cut -d' ' -f 2)
    if [ "$progress" = '100.00%' ]; then
        echo "$2 Late rate: $late_perc" >> late_rate_log.txt
    fi
    late_val_gt_late_rate_thresh=$(awk -v a="$late_val" -v b="$late_rate_thresh" 'BEGIN{print(a>b)}')
    if [ ! -z $late_val ] && [ $late_val_gt_late_rate_thresh -eq 1 ]; then
        echo "$2 Late rate > $late_rate_thresh %, Killing experiment"
        echo "$2 Late rate > $late_rate_thresh %, Killing experiment at Progress: $progress" >> late_rate_log.txt
        #sudo kill $1
        exit 1
        #sudo kill $$
    fi
done
