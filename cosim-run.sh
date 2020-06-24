#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

export LD_LIBRARY_PATH="$DIR/build/lib/:$LD_LIBRARY_PATH"

module=$1
example=$2
shift 2

exec build/src/$module/examples/ns3-dev-$example-debug "$@"
