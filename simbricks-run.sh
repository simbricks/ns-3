#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

target=$1
shift 1

exec ./ns3 run --no-build $target -- "$@"
