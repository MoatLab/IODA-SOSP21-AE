#!/bin/bash

wget http://people.cs.uchicago.edu/~huaicheng/ioda/ioda.qcow2
wget http://people.cs.uchicago.edu/~huaicheng/ioda/ioda.md5sum

echo "===> Checking integrity of the downloaded IODA VM image"
md5sum ioda.qcow2 > /tmp/ioda-local.md5sum
if [[ -n "$(diff /tmp/ioda-local.md5sum ioda.md5sum)" ]]; then
    echo ""
    echo "    => Error: IODA VM image corrupted ... please re-run the script"
    echo ""
fi
