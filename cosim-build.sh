#!/bin/bash
if [ "$COSIM_PATH" = "" ] ; then
  COSIM_PATH="/local/endhostsim/ehsim"
fi

export CPPFLAGS="-I$COSIM_PATH/lib"
export LDFLAGS="-L$COSIM_PATH/lib/simbricks/netif/ -lnetif_common"
if [ "$1" = "configure" ] ; then
  ./waf configure --enable-examples
fi
./waf build --enable-examples
