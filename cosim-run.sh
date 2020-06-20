#!/bin/bash
if [ "$COSIM_PATH" = "" ] ; then
  COSIM_PATH="/local/endhostsim/ehsim"
fi

export CPPFLAGS="-I$COSIM_PATH/proto -I$COSIM_PATH/netsim_common/include"
export LDFLAGS="-L$COSIM_PATH/netsim_common/ -lnetsim_common"

DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

./waf --run "src/cosim/examples/$*"
#./waf --run "src/cosim/examples/cosim-bridge-example" --command-template="gdb --args %s --CosimPort=/local/endhostsim/ehsim/experiments/out/qemu-ns3-bridge-pair/eth.a --CosimPort=/local/endhostsim/ehsim/experiments/out/qemu-ns3-bridge-pair/eth.b"
#./waf --run "src/cosim/examples/cosim-dumbbell-example" --command-template="gdb --args %s --CosimPortLeft=/local/endhostsim/ehsim/experiments/out/qemu-ns3-dumbbell-pair/eth.a --CosimPortRight=/local/endhostsim/ehsim/experiments/out/qemu-ns3-dumbbell-pair/eth.b"
