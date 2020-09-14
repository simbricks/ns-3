/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/sequencer-helper.h"
#include "ns3/cosim.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SequencerExample");

static std::vector<std::string> clientPortPaths;
static std::vector<std::string> serverPortPaths;
static std::vector<std::string> endhostSequencerPortPaths;

static bool
AddClientPort(std::string arg)
{
  clientPortPaths.push_back(arg);
  return true;
}

static bool
AddServerPort(std::string arg)
{
  serverPortPaths.push_back(arg);
  return true;
}

static bool
AddEndhostSequencerPort(std::string arg)
{
  endhostSequencerPortPaths.push_back(arg);
  return true;
}

int
main(int argc, char *argv[])
{
  LogComponentEnable("SequencerExample", LOG_LEVEL_INFO);
  LogComponentEnable("SequencerNetDevice", LOG_LEVEL_INFO);

  Time linkLatency(MicroSeconds(50));
  DataRate linkRate("1Gb/s");

  CommandLine cmd(__FILE__);
  cmd.AddValue("LinkLatency", "Propagation delay through links", linkLatency);
  cmd.AddValue("LinkRate", "Link bandwidth", linkRate);
  cmd.AddValue("ClientPort", "Add a client port to the switch",
          MakeCallback(&AddClientPort));
  cmd.AddValue("ServerPort", "Add a server port to the switch",
          MakeCallback(&AddServerPort));
  cmd.AddValue("EndhostSequencerPort", "Add a endhost sequencer port to the switch",
          MakeCallback(&AddEndhostSequencerPort));

  cmd.Parse(argc, argv);

  NS_ASSERT(clientPortPaths.size() > 0);
  NS_ASSERT(serverPortPaths.size() > 0);

  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

  NS_LOG_INFO("Create client and server nodes, and optionally endhost sequencer nodes");
  NodeContainer clientNodes;
  NodeContainer serverNodes;
  NodeContainer endhostSequencerNodes;
  clientNodes.Create(clientPortPaths.size());
  serverNodes.Create(serverPortPaths.size());
  if (!endhostSequencerPortPaths.empty()) {
    endhostSequencerNodes.Create(endhostSequencerPortPaths.size());
  }

  NS_LOG_INFO("Create switch node");
  NodeContainer switchNode;
  switchNode.Create(1);

  NS_LOG_INFO("Create simple channels");
  SimpleNetDeviceHelper simpleChannel;
  simpleChannel.SetChannelAttribute("Delay", TimeValue(linkLatency));
  simpleChannel.SetDeviceAttribute("DataRate", DataRateValue(linkRate));

  NetDeviceContainer clientDevices;
  NetDeviceContainer switchClientDevices;
  for (auto node = clientNodes.Begin(); node != clientNodes.End(); node++) {
    NetDeviceContainer link = simpleChannel.Install(NodeContainer(*node, switchNode));
    clientDevices.Add(link.Get(0));
    switchClientDevices.Add(link.Get(1));
  }
  NetDeviceContainer serverDevices;
  NetDeviceContainer switchServerDevices;
  for (auto node = serverNodes.Begin(); node != serverNodes.End(); node++) {
    NetDeviceContainer link = simpleChannel.Install(NodeContainer(*node, switchNode));
    serverDevices.Add(link.Get(0));
    switchServerDevices.Add(link.Get(1));
  }
  NetDeviceContainer endhostSequencerDevices;
  NetDeviceContainer switchEndhostSequencerDevices;
  for (auto node = endhostSequencerNodes.Begin();
      node != endhostSequencerNodes.End();
      node++) {
    NetDeviceContainer link = simpleChannel.Install(NodeContainer(*node, switchNode));
    endhostSequencerDevices.Add(link.Get(0));
    switchEndhostSequencerDevices.Add(link.Get(1));
  }

  NS_LOG_INFO("Create Switch");
  SequencerHelper sequencer;
  sequencer.Install(switchNode.Get(0),
                    switchServerDevices,
                    switchClientDevices,
                    switchEndhostSequencerDevices);

  NS_LOG_INFO("Create Cosims and Bridges");
  for (unsigned i = 0; i < clientNodes.GetN(); i++) {
    Ptr<CosimNetDevice> cosim = CreateObject<CosimNetDevice>();
    cosim->SetAttribute("UnixSocket", StringValue(clientPortPaths.at(i)));

    Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice>();
    bridge->SetAddress(Mac48Address::Allocate());

    clientNodes.Get(i)->AddDevice(cosim);
    clientNodes.Get(i)->AddDevice(bridge);
    bridge->AddBridgePort(clientDevices.Get(i));
    bridge->AddBridgePort(cosim);

    cosim->Start();
  }
  for (unsigned i = 0; i < serverNodes.GetN(); i++) {
    Ptr<CosimNetDevice> cosim = CreateObject<CosimNetDevice>();
    cosim->SetAttribute("UnixSocket", StringValue(serverPortPaths.at(i)));

    Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice>();
    bridge->SetAddress(Mac48Address::Allocate());

    serverNodes.Get(i)->AddDevice(cosim);
    serverNodes.Get(i)->AddDevice(bridge);
    bridge->AddBridgePort(serverDevices.Get(i));
    bridge->AddBridgePort(cosim);

    cosim->Start();
  }
  for (unsigned i = 0; i < endhostSequencerNodes.GetN(); i++) {
    Ptr<CosimNetDevice> cosim = CreateObject<CosimNetDevice>();
    cosim->SetAttribute("UnixSocket", StringValue(endhostSequencerPortPaths.at(i)));

    Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice>();
    bridge->SetAddress(Mac48Address::Allocate());

    endhostSequencerNodes.Get(i)->AddDevice(cosim);
    endhostSequencerNodes.Get(i)->AddDevice(bridge);
    bridge->AddBridgePort(endhostSequencerDevices.Get(i));
    bridge->AddBridgePort(cosim);

    cosim->Start();
  }

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
