#!/bin/bash

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
