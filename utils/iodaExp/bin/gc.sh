#!/bin/bash
#
# Get the IODA kernel level IO statistics and print the percentages
#

r=t.txt

getcnt > $r

bio_ttl=$(cat $r | grep BIO_TTL | awk -F: '{print $2}')
bio_gct_eio=$(cat $r | grep BIO_GCT_EIO | awk -F: '{print $2}')
bio_rfw_eio=$(cat $r | grep BIO_RFW_EIO | awk -F: '{print $2}')
pct_gct=$(echo "scale=6; $bio_gct_eio/$bio_ttl*100" | bc -l)
pct_rfw=$(echo "scale=6; $bio_rfw_eio/$bio_ttl*100" | bc -l)
pct=$(echo "scale=6; ($bio_gct_eio+$bio_rfw_eio)/$bio_ttl*100" | bc -l)

echo "GCT (%): ${pct_gct}"
echo "RFW (%): ${pct_rfw}"
echo "TTL (%): ${pct}"


dio_in_gc_num_0=$(cat $r | grep DIO_IN_GC_NUM-0 | awk -F: '{print $2}')
dio_in_gc_num_1=$(cat $r | grep DIO_IN_GC_NUM-1 | awk -F: '{print $2}')
dio_in_gc_num_2=$(cat $r | grep DIO_IN_GC_NUM-2 | awk -F: '{print $2}')
dio_in_gc_num_3=$(cat $r | grep DIO_IN_GC_NUM-3 | awk -F: '{print $2}')
dio_in_gc_num_4=$(cat $r | grep DIO_IN_GC_NUM-4 | awk -F: '{print $2}')
dio_tt=$(echo "scale=6; $dio_in_gc_num_0 + $dio_in_gc_num_1 + $dio_in_gc_num_2 + $dio_in_gc_num_3 + $dio_in_gc_num_4" | bc -l)

pct_gc0=$(echo "scale=6; $dio_in_gc_num_0/$dio_tt*100" | bc -l)
pct_gc1=$(echo "scale=6; $dio_in_gc_num_1/$dio_tt*100" | bc -l)
pct_gc2=$(echo "scale=6; $dio_in_gc_num_2/$dio_tt*100" | bc -l)
pct_gc3=$(echo "scale=6; $dio_in_gc_num_3/$dio_tt*100" | bc -l)
pct_gc4=$(echo "scale=6; $dio_in_gc_num_4/$dio_tt*100" | bc -l)

echo ""
echo -------DIO------------------
echo "0GC (%): $pct_gc0"
echo "1GC (%): $pct_gc1"
echo "2GC (%): $pct_gc2"
echo "3GC (%): $pct_gc3"
echo "4GC (%): $pct_gc4"



stripe_in_gc_num_0=$(cat $r | grep STRIPE_IN_GC_NUM-0 | awk -F: '{print $2}')
stripe_in_gc_num_1=$(cat $r | grep STRIPE_IN_GC_NUM-1 | awk -F: '{print $2}')
stripe_in_gc_num_2=$(cat $r | grep STRIPE_IN_GC_NUM-2 | awk -F: '{print $2}')
stripe_in_gc_num_3=$(cat $r | grep STRIPE_IN_GC_NUM-3 | awk -F: '{print $2}')
stripe_in_gc_num_4=$(cat $r | grep STRIPE_IN_GC_NUM-4 | awk -F: '{print $2}')
stripe_tt=$(echo "scale=6; $stripe_in_gc_num_0 + $stripe_in_gc_num_1 + $stripe_in_gc_num_2 + $stripe_in_gc_num_3 + $stripe_in_gc_num_4" | bc -l)

pct_stripe_gc0=$(echo "scale=6; $stripe_in_gc_num_0/$stripe_tt*100" | bc -l)
pct_stripe_gc1=$(echo "scale=6; $stripe_in_gc_num_1/$stripe_tt*100" | bc -l)
pct_stripe_gc2=$(echo "scale=6; $stripe_in_gc_num_2/$stripe_tt*100" | bc -l)
pct_stripe_gc3=$(echo "scale=6; $stripe_in_gc_num_3/$stripe_tt*100" | bc -l)
pct_stripe_gc4=$(echo "scale=6; $stripe_in_gc_num_4/$stripe_tt*100" | bc -l)

echo ""
echo -------stripe------------------
echo "0GC (%): $pct_stripe_gc0"
echo "1GC (%): $pct_stripe_gc1"
echo "2GC (%): $pct_stripe_gc2"
echo "3GC (%): $pct_stripe_gc3"
echo "4GC (%): $pct_stripe_gc4"
