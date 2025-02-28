#!/bin/bash

# Usage:
# ./fat_tree_run.sh <filename> <total_systems>

set -e
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR
export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

rm -rf $DIR/build/env/*
mkdir -p $DIR/build/env

./ns3 build

for (( c=0; c<$2; c++ ))
do 
./ns3 run $1 -- $c $2 $DIR/build/env/ &
echo "Process $c with PID $! started"
done

trap "killall ns3.38-$1-default; echo SIGINT; exit 1" INT
wait