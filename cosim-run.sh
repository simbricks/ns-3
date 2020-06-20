#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

example=$1
shift

exec build/src/cosim/examples/ns3-dev-$example-debug "$@"
