#!/bin/bash

# Usage:
# ./fat_tree_run.sh <filename> <total_systems>

# Function to get the current epoch time in seconds
get_epoch_time() {
    date +%s
}

set -e
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR
export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"
export SIMBRICKS_PATH="/simbricks"

rm -rf $DIR/build/env/*
mkdir -p $DIR/build/env

./ns3 build

start_time=$(get_epoch_time)
echo "START TIME: $start_time"

for (( c=0; c<$2; c++ ))
do 
./ns3 run $1 -- $c $2 $DIR/build/env/ $3&
echo "Process $c with PID $! started"
done


trap "killall ns3.38-$1-default; echo SIGINT; exit 1" INT
wait

end_time=$(get_epoch_time)
echo "END TIME: $end_time"
duration=$((end_time-start_time))
echo "Run Time: $duration seconds"
