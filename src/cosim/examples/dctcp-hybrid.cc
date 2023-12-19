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
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/bridge-module.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/cosim.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SplitSimHybridExample");

std::vector<std::string> cosimLeftPaths;
std::vector<std::string> cosimRightPaths;
std::vector<uint64_t> DummyRecvBytes;
std::ofstream DummyRecvTput;

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

void
TraceS1R1Sink (std::size_t index, Ptr<const Packet> p, const Address& a)
{
  DummyRecvBytes[index] += p->GetSize ();
}


void
InitializeCounters (size_t num_pairs)
{
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      DummyRecvBytes[i] = 0;
    }

}

void
PrintThroughput (Time tputInterval, size_t num_pairs)
{

  for (std::size_t i = 0; i < num_pairs; i++)
    {
      DummyRecvTput << Simulator::Now ().GetSeconds () << "s " << i << " " << (DummyRecvBytes[i] * 8) / 1e6 << std::endl;
    }
  InitializeCounters(num_pairs);
  Simulator::Schedule (tputInterval, &PrintThroughput, tputInterval, num_pairs);
}


void
PrintProgress (Time interval)
{
  NS_LOG_INFO( "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << " seconds simulation time" << std::endl);
  Simulator::Schedule (interval, &PrintProgress, interval);
}

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



int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::Unit::PS);

  Time linkLatency(MilliSeconds (10));
  DataRate linkRate("10Mb/s");
  double ecnTh = 200000;
  int dummy_num_pairs = 1;
  int detail_num_pairs = 1;
  std::string tcpTypeId = "TcpDctcp";
  uint32_t mtu = 1500;  

  Time flowStartupWindow = MilliSeconds (500);
  Time convergenceTime = MilliSeconds (500);
  Time measurementWindow = Seconds (8);
  Time progressInterval = MilliSeconds (500);
  Time tputInterval = Seconds (1);
  
  CommandLine cmd (__FILE__);
  cmd.AddValue ("LinkLatency", "Propagation delay through link", linkLatency);
  cmd.AddValue ("LinkRate", "Link bandwidth", linkRate);
  cmd.AddValue ("EcnTh", "ECN Threshold queue size", ecnTh);
  cmd.AddValue ("mtu", "Ethernet mtu", mtu);
  cmd.AddValue ("CosimPortLeft", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimLeftPort));
  cmd.AddValue ("CosimPortRight", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimRightPort));
  cmd.Parse (argc, argv);

  int total_num_pairs = dummy_num_pairs + detail_num_pairs;
  Time startTime = Seconds (1);
  Time stopTime = startTime + flowStartupWindow + convergenceTime + measurementWindow;
  DummyRecvBytes.reserve(dummy_num_pairs);
  Time TputStartTime = startTime + flowStartupWindow + convergenceTime + tputInterval;

  LogComponentEnable("SplitSimHybridExample", LOG_LEVEL_ALL);
  // LogComponentEnable("CosimNetDevice", LOG_LEVEL_ALL);
  // LogComponentEnable("BridgeNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable("CosimDumbbellExample", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleChannel", LOG_LEVEL_ALL);
  //LogComponentEnable("SimpleNetDevice", LOG_LEVEL_INFO);
  //LogComponentEnable ("RedQueueDisc", LOG_LEVEL_ALL);
  //LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
  //LogComponentEnable ("DevRedQueue", LOG_LEVEL_ALL);
  //LogComponentEnable ("Queue", LOG_LEVEL_ALL);
  //LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_ALL);

  LogComponentEnableAll(LOG_PREFIX_TIME);
  LogComponentEnableAll(LOG_PREFIX_NODE);

  // Configurations for dummy hosts in ns3 
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (mtu-52));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
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
  ptpChan->SetAttribute ("Delay", TimeValue (linkLatency));

  SimpleNetDeviceHelper pointToPointSR;
  pointToPointSR.SetQueue("ns3::DevRedQueue", "MaxSize", StringValue("2666p"));
  pointToPointSR.SetQueue("ns3::DevRedQueue", "MinTh", DoubleValue (ecnTh));
  pointToPointSR.SetDeviceAttribute ("DataRate", DataRateValue(linkRate));
  pointToPointSR.SetChannelAttribute ("Delay", TimeValue (linkLatency));

  SimpleNetDeviceHelper pointToPointHost;
  pointToPointHost.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPointHost.SetChannelAttribute ("Delay", TimeValue (linkLatency));

  NetDeviceContainer ptpDev = pointToPointSR.Install (nodes, ptpChan);
  NS_LOG_INFO ("num node device" << nodeLeft->GetNDevices() << " type: ");
  bridgeLeft->AddBridgePort (nodeLeft->GetDevice(1));  
  bridgeRight->AddBridgePort (nodeRight->GetDevice(1));

  BridgeHelper bridge;



  // Add dummy ns3 hosts
  NS_LOG_INFO ("Create dummy ns3 hosts and add them to bridge");
  NodeContainer DumLeftNode, DumRightNode;
  DumLeftNode.Create(dummy_num_pairs);
  DumRightNode.Create(dummy_num_pairs);
  
  std::vector<NetDeviceContainer> DumLeftDev;
  DumLeftDev.reserve (dummy_num_pairs);
  std::vector<NetDeviceContainer> DumRightDev;
  DumRightDev.reserve (dummy_num_pairs);

  for (int i = 0; i < dummy_num_pairs; i++){
    // Add left side
    Ptr<Node> left_host_node = DumLeftNode.Get(i);
    Ptr<SimpleChannel> ptpChanL = CreateObject<SimpleChannel> ();
    ptpChanL->SetAttribute ("Delay", TimeValue (linkLatency));
    pointToPointHost.Install(left_host_node, ptpChanL);
    // add the netdev to bridge port
    bridgeLeft->AddBridgePort(pointToPointHost.Install(nodeLeft, ptpChanL).Get(0));
    
    // Add right side
    Ptr<Node> right_host_node = DumRightNode.Get(i);
    Ptr<SimpleChannel> ptpChanR = CreateObject<SimpleChannel> ();
    ptpChanR->SetAttribute ("Delay", TimeValue (linkLatency));
    pointToPointHost.Install(right_host_node, ptpChanR);
    // add the netdev to bridge port
    bridgeRight->AddBridgePort(pointToPointHost.Install(nodeRight, ptpChanR).Get(0));
  }

  // Network configurations for ns3 hosts
  InternetStackHelper stack;
  stack.Install(DumLeftNode);
  stack.Install(DumRightNode);

  std::vector<Ipv4InterfaceContainer> ipRight;
  ipRight.reserve (dummy_num_pairs);

  Ipv4AddressHelper ipv4;
  std::string base_ip  = "0.0.0." + std::to_string(detail_num_pairs + 1); 
  ipv4.SetBase ("192.168.64.0", "255.255.255.0", base_ip.c_str() );
  for ( int i = 0; i < dummy_num_pairs; i++){
    Ipv4InterfaceContainer le;
    le = ipv4.Assign(DumLeftNode.Get(i)->GetDevice(0));
    NS_LOG_INFO ("Left IP: " << le.GetAddress(0));
  }
  
  base_ip  = "0.0.0." + std::to_string(detail_num_pairs + 1 + total_num_pairs);
  ipv4.SetBase ("192.168.64.0", "255.255.255.0", base_ip.c_str());
  for ( int i = 0; i < dummy_num_pairs; i++){
    ipRight.push_back(ipv4.Assign(DumRightNode.Get(i)->GetDevice(0)));
    NS_LOG_INFO ("Right IP: " << ipRight[i].GetAddress(0));
  }
  
  // Create MyApp to ns3 hosts 
  std::vector<Ptr<PacketSink> > RightSinks;
  RightSinks.reserve (dummy_num_pairs);

  std::vector<Ptr<Socket>> sockets;
  sockets.reserve(dummy_num_pairs);

  for (int i = 0; i < dummy_num_pairs; i++)
    {
      uint16_t port = 50000 + i;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (DumRightNode.Get (i));
      Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
      RightSinks.push_back (packetSink);
      sinkApp.Start (startTime);
      sinkApp.Stop (stopTime);

      Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (DumLeftNode.Get (i), TcpSocketFactory::GetTypeId ());
      sockets.push_back(ns3TcpSocket);

      Ptr<MyApp> app = CreateObject<MyApp> ();
      app->Setup (ns3TcpSocket, InetSocketAddress(ipRight[i].GetAddress(0), port), mtu-52, 1000000000, DataRate ("10Gbps"));
      DumLeftNode.Get (i)->AddApplication (app);
      app->SetStartTime (startTime);
      app->SetStopTime (stopTime);

    }

  std::ostringstream tput;
  tput << "./out/dctcp-hybr-tput-" << mtu << "-" << ecnTh <<  ".dat";

  DummyRecvTput.open (tput.str(), std::ios::out);
  DummyRecvTput << "#Time(s) flow thruput(Mb/s) MTU: " << mtu << " K_value: " << ecnTh << std::endl;

  for (std::size_t i = 0; i < dummy_num_pairs; i++) {
    RightSinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS1R1Sink, i));
  }
  Simulator::Schedule (startTime + flowStartupWindow + convergenceTime, &InitializeCounters, dummy_num_pairs);
  Simulator::Schedule (TputStartTime, &PrintThroughput, tputInterval, dummy_num_pairs);
  Simulator::Schedule (progressInterval, &PrintProgress, progressInterval);

  NS_LOG_INFO ("Create detailed hosts and add them to bridge");
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

  
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Run ();

  DummyRecvTput.close();

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
