/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington, 2012 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <iomanip>

#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simbricks-netdev.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimbricksDumbbellExample");

std::vector<std::string> simbricksLeftPaths;
std::vector<std::string> simbricksRightPaths;


bool AddSimbricksLeftPort (const std::string& arg)
{
  simbricksLeftPaths.push_back (arg);
  return true;
}

bool AddSimbricksRightPort (const std::string& arg)
{
  simbricksRightPaths.push_back (arg);
  return true;
}



int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::Unit::PS);

  Time linkLatency(MilliSeconds (10));
  DataRate linkRate("10Mb/s");
  double ecnTh = 200000;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("LinkLatency", "Propagation delay through link", linkLatency);
  cmd.AddValue ("LinkRate", "Link bandwidth", linkRate);
  cmd.AddValue ("EcnTh", "ECN Threshold queue size", ecnTh);
  cmd.AddValue ("SimbricksPortLeft", "Add a simbricks ethernet port to the bridge",
      MakeCallback (&AddSimbricksLeftPort));
  cmd.AddValue ("SimbricksPortRight", "Add a simbricks ethernet port to the bridge",
      MakeCallback (&AddSimbricksRightPort));
  cmd.Parse (argc, argv);

  //LogComponentEnable("SimbricksNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable("BridgeNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable("SimbricksDumbbellExample", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleChannel", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleNetDevice", LOG_LEVEL_INFO);
  //LogComponentEnable ("RedQueueDisc", LOG_LEVEL_ALL);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
  //LogComponentEnable ("DevRedQueue", LOG_LEVEL_ALL);
  //LogComponentEnable ("Queue", LOG_LEVEL_ALL);
  //LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_ALL);

  LogComponentEnableAll(LOG_PREFIX_TIME);
  LogComponentEnableAll(LOG_PREFIX_NODE);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  
  NS_LOG_INFO ("Create Nodes");
  Ptr<Node> nodeLeft = CreateObject<Node> ();
  Ptr<Node> nodeRight = CreateObject<Node> ();
  NodeContainer nodes (nodeLeft);
  nodes.Add(nodeRight);
  NS_LOG_INFO ("Node Num: " << nodes.GetN());

  NS_LOG_INFO ("Create BridgeDevice");
  Ptr<BridgeNetDevice> bridgeLeft = CreateObject<BridgeNetDevice> ();
  Ptr<BridgeNetDevice> bridgeRight = CreateObject<BridgeNetDevice> ();
  bridgeLeft->SetAddress (Mac48Address::Allocate ());
  bridgeRight->SetAddress (Mac48Address::Allocate ());
  nodeLeft->AddDevice (bridgeLeft);
  nodeRight->AddDevice (bridgeRight);

  NS_LOG_INFO ("Create simple channel link between the two");
  Ptr<SimpleChannel> ptpChan = CreateObject<SimpleChannel> ();

  SimpleNetDeviceHelper pointToPointSR;
  pointToPointSR.SetQueue("ns3::DevRedQueue", "MaxSize", StringValue("2666p"));
  pointToPointSR.SetQueue("ns3::DevRedQueue", "MinTh", DoubleValue (ecnTh));
  pointToPointSR.SetDeviceAttribute ("DataRate", DataRateValue(linkRate));
  pointToPointSR.SetChannelAttribute ("Delay", TimeValue (linkLatency));

  //ptpChan->SetAttribute ("Delay", TimeValue (linkLatency));

  //Ptr<SimpleNetDevice> ptpDevLeft = CreateObject<SimpleNetDevice> ();
  //Ptr<SimpleNetDevice> ptpDevRight = CreateObject<SimpleNetDevice> ();
  //Ptr<PointToPointNetDevice> ptpDevLeft = CreateObject<PointToPointNetDevice> ();
  //Ptr<PointToPointNetDevice> ptpDevRight = CreateObject<PointToPointNetDevice> ();
  //ptpDevLeft = pointToPointSR.Install (nodeLeft, nodeRight).Get(0);
  //ptpDevRight = pointToPointSR.Install (nodeLeft, nodeRight).Get(1);
  //NetDeviceContainer ptpDev = pointToPointSR.Install (nodeLeft, nodeRight);

  NetDeviceContainer ptpDev = pointToPointSR.Install (nodes, ptpChan);

 
  //ptpDevLeft->SetAttribute ("DataRate", DataRateValue(linkRate));
  //ptpDevRight->SetAttribute ("DataRate", DataRateValue(linkRate));
  //ptpDevLeft->SetAddress (Mac48Address::Allocate ());
  //ptpDevRight->SetAddress (Mac48Address::Allocate ());
  //ptpChan->Add (ptpDevLeft);
  //ptpChan->Add (ptpDevRight);
  //ptpDevLeft->SetChannel (ptpChan);
  //ptpDevRight->SetChannel (ptpChan);
  //nodeLeft->AddDevice (ptpDevLeft);
  //nodeRight->AddDevice (ptpDevRight);
  //bridgeLeft->AddBridgePort (ptpDevLeft);
  //bridgeRight->AddBridgePort (ptpDevRight);
  NS_LOG_INFO ("num node device" << nodeLeft->GetNDevices() << " type: ");
  bridgeLeft->AddBridgePort (nodeLeft->GetDevice(1));
  
  bridgeRight->AddBridgePort (nodeRight->GetDevice(1));

  NS_LOG_INFO ("Create SimbricksDevices and add them to bridge");
  NS_LOG_INFO("simbricks path :" << simbricksLeftPaths[0]);
  
  for (std::string& cpp : simbricksLeftPaths) {
    Ptr<simbricks::SimbricksNetDevice> device =
        CreateObject<simbricks::SimbricksNetDevice> ();
    device->SetAttribute ("UnixSocket", StringValue (cpp));
    nodeLeft->AddDevice (device);
    bridgeLeft->AddBridgePort (device);
    device->Start();
  }
  for (std::string& cpp : simbricksRightPaths) {
    Ptr<simbricks::SimbricksNetDevice> device =
        CreateObject<simbricks::SimbricksNetDevice> ();
    device->SetAttribute ("UnixSocket", StringValue (cpp));
    nodeRight->AddDevice (device);
    bridgeRight->AddBridgePort (device);
    device->Start();
  }

  
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Run ();


  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
