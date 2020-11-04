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

#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
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

  CommandLine cmd (__FILE__);
  cmd.AddValue ("LinkLatency", "Propagation delay through link", linkLatency);
  cmd.AddValue ("LinkRate", "Link bandwidth", linkRate);
  cmd.AddValue ("CosimPortLeft", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimLeftPort));
  cmd.AddValue ("CosimPortRight", "Add a cosim ethernet port to the bridge",
      MakeCallback (&AddCosimRightPort));
  cmd.Parse (argc, argv);

  /*LogComponentEnable("CosimNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable("BridgeNetDevice", LOG_LEVEL_ALL);*/

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NS_LOG_INFO ("Create Nodes");
  Ptr<Node> nodeLeft = CreateObject<Node> ();
  Ptr<Node> nodeRight = CreateObject<Node> ();

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

  Ptr<SimpleNetDevice> ptpDevLeft = CreateObject<SimpleNetDevice> ();
  Ptr<SimpleNetDevice> ptpDevRight = CreateObject<SimpleNetDevice> ();
  ptpDevLeft->SetAttribute ("DataRate", DataRateValue(linkRate));
  ptpDevRight->SetAttribute ("DataRate", DataRateValue(linkRate));
  ptpDevLeft->SetAddress (Mac48Address::Allocate ());
  ptpDevRight->SetAddress (Mac48Address::Allocate ());
  ptpChan->Add (ptpDevLeft);
  ptpChan->Add (ptpDevRight);
  ptpDevLeft->SetChannel (ptpChan);
  ptpDevRight->SetChannel (ptpChan);
  nodeLeft->AddDevice (ptpDevLeft);
  nodeRight->AddDevice (ptpDevRight);
  bridgeLeft->AddBridgePort (ptpDevLeft);
  bridgeRight->AddBridgePort (ptpDevRight);


  NS_LOG_INFO ("Create CosimDevices and add them to bridge");
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
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
