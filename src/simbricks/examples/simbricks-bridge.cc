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
#include "ns3/simbricks-netdev.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimbricksBridgeExample");

std::vector<std::string> simbricksPortPaths;

bool AddSimbricksPort (const std::string& arg)
{
  simbricksPortPaths.push_back (arg);
  return true;
}

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::Unit::PS);

  CommandLine cmd (__FILE__);
  cmd.AddValue ("SimbricksPort", "Add a simbricks ethernet port to the bridge",
      MakeCallback (&AddSimbricksPort));
  cmd.Parse (argc, argv);

  //LogComponentEnable("SimbricksNetDevice", LOG_LEVEL_ALL);

  Ipv4Address hostIp ("192.168.64.253");
  Ipv4Mask hostMask ("255.255.255.0");

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NS_LOG_INFO ("Create Node");
  Ptr<Node> node = CreateObject<Node> ();

  NS_LOG_INFO ("Create BridgeDevice");
  Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice> ();
  bridge->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (bridge);

  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (node);

  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (bridge);
  Ipv4InterfaceAddress address = Ipv4InterfaceAddress (hostIp, hostMask);
  ipv4->AddAddress (interface, address);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);

  NS_LOG_INFO ("Create SimbricksDevices and add them to bridge");
  std::vector <Ptr<simbricks::SimbricksNetDevice>> simbricksDevs;
  for (std::string cpp : simbricksPortPaths) {
    Ptr<simbricks::SimbricksNetDevice> device =
      CreateObject<simbricks::SimbricksNetDevice> ();
    device->SetAttribute ("UnixSocket", StringValue (cpp));
    node->AddDevice (device);
    bridge->AddBridgePort (device);
    device->Start();
  }

  NS_LOG_INFO ("Run Emulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
