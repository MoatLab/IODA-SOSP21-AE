#!/bin/bash

usage() {
    echo ""
    echo "Usage: $0 [input] [mode] [rate]"
    echo ""
}

if [[ $# != 3 ]]; then
    usage
    exit
fi


