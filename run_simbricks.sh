#!/bin/bash

# Usage:
# ./run_sim.sh <filename> <total_systems>

set -e
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR
export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

rm -rf $DIR/build/env/*
mkdir -p $DIR/build/env

./waf build

for (( c=0; c<$2; c++ ))
do 
$DIR/build/scratch/$1 $c $2 $DIR/build/env/ &
echo "Process $c with PID $! started"
done

trap "killall $1; echo SIGINT; exit 1" INT
wait