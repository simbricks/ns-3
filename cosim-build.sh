#!/bin/bash
if [ "$COSIM_PATH" = "" ] ; then
  COSIM_PATH="/local/endhostsim/ehsim"
fi

export CPPFLAGS="-I$COSIM_PATH/proto -I$COSIM_PATH/netsim_common/include"
export LDFLAGS="-L$COSIM_PATH/netsim_common/ -lnetsim_common"
if [ "$1" = "configure" ] ; then
  ./waf configure --enable-examples
fi
./waf build --enable-examples
