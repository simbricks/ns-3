#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

#exec build/examples/tcp/ns3-dev-dctcp-modes-debug --EcnTh=$1 --mtu=$2
exec build/examples/tcp/ns3-dev-dctcp-cwnd-devred-debug  --EcnTh=$1 --mtu=$2 --LinkLatency=$3
