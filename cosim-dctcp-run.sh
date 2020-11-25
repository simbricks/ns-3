#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

exec build/scratch/dctcp-modes --EcnTh=$1 --mtu=$2