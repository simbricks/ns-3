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
#include "ns3/cosim.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CosimDumbbellExample");

std::vector<std::string> cosimLeftPaths;
std::vector<std::string> cosimRightPaths;

std::ofstream LeftQueueLength;
std::ofstream Qstat;

bool AddCosimLeftPort (std::string arg)
{
  cosimLeftPaths.push_back (arg);
  return true;
}

bool AddCosimRightPort (std::string arg)
{
  cosimRightPaths.push_back (arg);
  return true;
}


void
CheckLeftQueueSize (Ptr<QueueDisc> queue)
{
  // 1500 byte packets
  uint32_t qSize = queue->GetNPackets ();
  Time backlog = Seconds (static_cast<double> (qSize * 1500 * 8) / 1e10); // 10 Gb/s
  // report size in units of packets and ms
  LeftQueueLength << std::fixed << std::setprecision (9) << Simulator::Now ().GetSeconds () << " " << qSize << " " << backlog.GetMicroSeconds () << std::endl;
  // check queue size every 1/100 of a second
  Simulator::Schedule (MilliSeconds (1), &CheckLeftQueueSize, queue);
}


void
CheckQstat(Ptr<QueueDisc> Disc_0, Ptr<QueueDisc> Disc_1){
  
  QueueDisc::Stats st = Disc_0->GetStats ();
  Qstat << "*** RED stats from Node 0 queue disc ***" << std::endl;
  Qstat << st << std::endl;

  st = Disc_1->GetStats ();
  Qstat << "*** RED stats from Node 1 queue disc ***" << std::endl;
  Qstat << st << std::endl;
  Simulator::Schedule (MilliSeconds (10), &CheckQstat, Disc_0, Disc_1);

}

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::Unit::PS);

  Time linkLatency(MilliSeconds (10));
  DataRate linkRate("10Mb/s");

  CommandLine cmd (__FILE__);
  cmd.AddValue ("LinkLatency", "Propagation delay through link", linkLatency);
  cmd.AddValue ("LinkRate", "Link bandwidth", linkRate);
  cmd.AddValue ("CosimPortLeft", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimLeftPort));
  cmd.AddValue ("CosimPortRight", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimRightPort));
  cmd.Parse (argc, argv);

  //LogComponentEnable("CosimNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable("BridgeNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable("CosimDumbbellExample", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleChannel", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable ("RedQueueDisc", LOG_LEVEL_ALL);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
  LogComponentEnable ("DevRedQueue", LOG_LEVEL_ALL);
  //LogComponentEnable ("Queue", LOG_LEVEL_ALL);
  //LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_ALL);

  LogComponentEnableAll(LOG_PREFIX_TIME);
  LogComponentEnableAll(LOG_PREFIX_NODE);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

// Set default parameters for RED queue disc
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("2666p")));
// DCTCP tracks instantaneous queue length only; so set QW = 1
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (10));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (15));


  
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
  //Ptr<PointToPointChannel> ptpChan = CreateObject<PointToPointChannel> ();
  
  //PointToPointHelper pointToPointSR;
  SimpleNetDeviceHelper pointToPointSR;
  pointToPointSR.SetQueue("ns3::DevRedQueue", "MaxSize", StringValue("100p"));
  //pointToPointSR.SetQueue("ns3::RedQueueDisc", "MaxSize", StringValue("100p"));
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
  

  TrafficControlHelper tchRed10;
  // MinTh = 50, MaxTh = 150 recommended in ACM SIGCOMM 2010 DCTCP Paper
  // This yields a target (MinTh) queue depth of 60us at 10 Gb/s
  tchRed10.SetRootQueueDisc ("ns3::RedQueueDisc",
                             "LinkBandwidth", DataRateValue(linkRate),
                             "LinkDelay", TimeValue (linkLatency),
                             "MinTh", DoubleValue (1),
                             "MaxTh", DoubleValue (1));

  InternetStackHelper stack;
  stack.InstallAll ();

  QueueDiscContainer queueDiscs1 = tchRed10.Install (ptpDev);

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

  NS_LOG_INFO ("Create CosimDevices and add them to bridge");
  NS_LOG_INFO("cosim path :" << cosimLeftPaths[0]);
  
  for (std::string cpp : cosimLeftPaths) {
    Ptr<CosimNetDevice> device = CreateObject<CosimNetDevice> ();
    device->SetAttribute ("UnixSocket", StringValue (cpp));
    nodeLeft->AddDevice (device);
    bridgeLeft->AddBridgePort (device);
    device->Start ();
  }
  for (std::string cpp : cosimRightPaths) {
    Ptr<CosimNetDevice> device = CreateObject<CosimNetDevice> ();
    device->SetAttribute ("UnixSocket", StringValue (cpp));
    nodeRight->AddDevice (device);
    bridgeRight->AddBridgePort (device);
    device->Start ();
  }

  
  //AsciiTraceHelper ascii;
  //pointToPointSR.EnableAsciiAll (ascii.CreateFileStream ("cosim.tr"));
  //pointToPointSR.EnablePcap ("cosim", ptpDev, 0);
  LeftQueueLength.open ("cosim-left-length.dat", std::ios::out);
  Qstat.open ("cosim-qstat.dat", std::ios::out);
  Simulator::Schedule (Seconds (0.0), &CheckLeftQueueSize, queueDiscs1.Get (0));
  Simulator::Schedule (Seconds (0.0), &CheckQstat, queueDiscs1.Get(0), queueDiscs1.Get(1));
  
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Run ();


  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
