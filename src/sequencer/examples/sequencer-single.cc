/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/sequencer-helper.h"
#include "ns3/cosim.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SequencerExample");

int
main(int argc, char *argv[])
{
  LogComponentEnable("SequencerExample", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  Time linkLatency(MicroSeconds(50));
  DataRate linkRate("1Gb/s");

  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

  NS_LOG_INFO("Create Client and Server Nodes");
  NodeContainer terminals;
  terminals.Create(2);

  NS_LOG_INFO("Create Switch Node");
  NodeContainer switchNode;
  switchNode.Create(1);

  NS_LOG_INFO("Create simple channel");
  SimpleNetDeviceHelper simpleChannel;
  simpleChannel.SetChannelAttribute("Delay", TimeValue(linkLatency));
  simpleChannel.SetDeviceAttribute("DataRate", DataRateValue(linkRate));

  NetDeviceContainer terminalDevices;
  NetDeviceContainer switchDevices;
  for (int i = 0; i < 2; i++) {
    NetDeviceContainer link = simpleChannel.Install(NodeContainer(terminals.Get(i), switchNode));
    terminalDevices.Add(link.Get(0));
    switchDevices.Add(link.Get(1));
  }

  NS_LOG_INFO("Create Switch");
  SequencerHelper sequencer;
  sequencer.Install(switchNode.Get(0), switchDevices, true);

  NS_LOG_INFO("Create Cosims and Bridges");
  for (int i = 0; i < 2; i++) {
    Ptr<CosimNetDevice> cosim = CreateObject<CosimNetDevice>();
    cosim->SetAttribute("Sync", BooleanValue(false));
    char path[64];
    sprintf(path, "/tmp/cosim-eth-%d", i);
    cosim->SetAttribute("UnixSocket", StringValue(path));

    Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice>();
    bridge->SetAddress(Mac48Address::Allocate());

    terminals.Get(i)->AddDevice(cosim);
    terminals.Get(i)->AddDevice(bridge);
    bridge->AddBridgePort(terminalDevices.Get(i));
    bridge->AddBridgePort(cosim);

    cosim->Start();
  }

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
