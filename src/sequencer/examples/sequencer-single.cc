/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/sequencer-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SequencerExample");

int
main(int argc, char *argv[])
{
  LogComponentEnable("SequencerExample", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  Time linkLatency(MilliSeconds (10));
  DataRate linkRate("10Mb/s");

  GlobalValue::Bind("ChecksumEnabled", BooleanValue (true));

  NS_LOG_INFO("Create Client and Server Nodes");
  NodeContainer terminals;
  terminals.Create (2);

  NS_LOG_INFO("Create Switch Node");
  NodeContainer switchNode;
  switchNode.Create (1);

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
  sequencer.Install(switchNode.Get(0), switchDevices);

  NS_LOG_INFO("Install Stack and Assign Addresses");
  InternetStackHelper internet;
  internet.Install(terminals);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign(terminalDevices);

  NS_LOG_INFO("Create Application");
  UdpEchoServerHelper echoServer(12345);
  ApplicationContainer serverApp = echoServer.Install(terminals.Get(0));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 12345);
  echoClient.SetAttribute("MaxPackets", UintegerValue(10));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApp = echoClient.Install(terminals.Get(1));
  clientApp.Start(Seconds(2.0));
  clientApp.Stop(Seconds(10.0));

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
