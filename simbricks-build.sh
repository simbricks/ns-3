#!/bin/bash
if [ "$SIMBRICKS_PATH" = "" ] ; then
  echo "Please set environment variable SIMBRICKS_PATH before building ns3"
  exit 1
fi

DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

if [ "$1" = "configure" ] ; then
  ./ns3 clean
  ./ns3 configure --build-profile=default --enable-examples
fi
./ns3 build