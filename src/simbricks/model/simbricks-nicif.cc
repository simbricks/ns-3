/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2020 Max Planck Institute for Software Systems
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "simbricks-nicif.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/integer.h"
#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimbricksNetDeviceNicIf");

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
TypeId SimbricksNetDeviceNicIf::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimbricksNetDeviceNicIf")
    .SetParent<NetDevice> ()
    .SetGroupName ("SimbricksNetDeviceNicIf")
    .AddConstructor<SimbricksNetDeviceNicIf> ()
    .AddAttribute ("UnixSocket",
                   "The path to the Ethernet Unix socket",
                   StringValue ("/tmp/simbricks-eth"),
                   MakeStringAccessor (&SimbricksNetDeviceNicIf::m_a_uxSocketPath),
                   MakeStringChecker ())
    .AddAttribute ("SyncDelay",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksNetDeviceNicIf::m_a_syncDelay),
                   MakeTimeChecker ())
    .AddAttribute ("PollDelay",
                   "Delay between polling for messages in non-sync mode",
                   TimeValue (NanoSeconds (100.)),
                   MakeTimeAccessor (&SimbricksNetDeviceNicIf::m_a_pollDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EthLatency",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksNetDeviceNicIf::m_a_ethLatency),
                   MakeTimeChecker ())
    .AddAttribute ("Sync",
                   "Request synchronous interaction with device",
                   IntegerValue (1),
                   MakeIntegerAccessor (&SimbricksNetDeviceNicIf::m_a_sync),
                   MakeIntegerChecker<int32_t> ())
    .AddAttribute ("SyncMode",
                   "Set synchronous mode",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimbricksNetDeviceNicIf::m_a_sync_mode),
                   MakeBooleanChecker ())
    ;
    return tid;
}

SimbricksNetDeviceNicIf::SimbricksNetDeviceNicIf ()
  : m_mtu(1500), m_node(0)
{
  NS_LOG_FUNCTION_NOARGS ();
  // Nullifying callbacks explicitly is probably not needed
  m_rxCallback.Nullify ();
  m_promiscRxCallback.Nullify ();
}

SimbricksNetDeviceNicIf::~SimbricksNetDeviceNicIf ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void SimbricksNetDeviceNicIf::Start ()
{
  SimbricksNetIfDefaultParams(&m_adapter.m_bifparam);
  
  m_adapter.m_bifparam.sock_path = m_a_uxSocketPath.c_str();
  m_a_shmPath = m_a_uxSocketPath + "-shm";
  NS_LOG_INFO (m_a_shmPath);

  m_adapter.m_bifparam.sync_interval = m_a_syncDelay.ToInteger (Time::PS);
  m_adapter.m_pollDelay = m_a_pollDelay;
  m_adapter.m_bifparam.link_latency = m_a_ethLatency.ToInteger (Time::PS);
  m_adapter.m_bifparam.sync_mode = (enum SimbricksBaseIfSyncMode)m_a_sync;
  m_adapter.m_shmPath = m_a_shmPath;
  m_adapter.SetReceiveCallback (
      MakeCallback (&SimbricksNetDeviceNicIf::AdapterRx, this));
  m_adapter.Start ();
}

void SimbricksNetDeviceNicIf::Stop ()
{
  m_adapter.Stop ();
}

void SimbricksNetDeviceNicIf::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t SimbricksNetDeviceNicIf::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel> SimbricksNetDeviceNicIf::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}

void SimbricksNetDeviceNicIf::SetAddress (Address address)
{
  NS_LOG_FUNCTION (address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address SimbricksNetDeviceNicIf::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool SimbricksNetDeviceNicIf::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t SimbricksNetDeviceNicIf::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}

bool SimbricksNetDeviceNicIf::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDeviceNicIf::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool SimbricksNetDeviceNicIf::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address SimbricksNetDeviceNicIf::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool SimbricksNetDeviceNicIf::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address SimbricksNetDeviceNicIf::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimbricksNetDeviceNicIf::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool SimbricksNetDeviceNicIf::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool SimbricksNetDeviceNicIf::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool SimbricksNetDeviceNicIf::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool SimbricksNetDeviceNicIf::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << source << dest << protocolNumber);

  EthernetHeader header (false);
  header.SetSource (Mac48Address::ConvertFrom (source));
  header.SetDestination (Mac48Address::ConvertFrom (dest));
  header.SetLengthType (protocolNumber);

  packet->AddHeader (header);

  m_adapter.Transmit (packet);
  return true;
}

Ptr<Node> SimbricksNetDeviceNicIf::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void SimbricksNetDeviceNicIf::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);
  m_node = node;
}

bool SimbricksNetDeviceNicIf::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDeviceNicIf::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void SimbricksNetDeviceNicIf::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool SimbricksNetDeviceNicIf::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDeviceNicIf::AdapterRx (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t nid = m_node->GetId ();
  Simulator::ScheduleWithContext (nid, Seconds (0.0),
      MakeEvent (&SimbricksNetDeviceNicIf::RxInContext, this, packet));
}

void SimbricksNetDeviceNicIf::RxInContext (Ptr<Packet> packet)
{
  PacketType packetType;
  Mac48Address destination;
  Mac48Address source;
  uint16_t protocol;

  EthernetHeader header (false);

  // packet is shorter than header -> drop
  if (packet->GetSize () < header.GetSerializedSize ())
    return;

  packet->RemoveHeader (header);

  destination = header.GetDestination ();
  source = header.GetSource ();
  protocol = header.GetLengthType ();

  if (header.GetDestination ().IsBroadcast ())
    packetType = NS3_PACKET_BROADCAST;
  else if (header.GetDestination ().IsGroup ())
    packetType = NS3_PACKET_MULTICAST;
  else if (destination == m_address)
    packetType = NS3_PACKET_HOST;
  else
    packetType = NS3_PACKET_OTHERHOST;

  if (!m_promiscRxCallback.IsNull ()) {
    m_promiscRxCallback (this, packet, protocol, source, destination,
        packetType);
  }

  if (packetType != NS3_PACKET_OTHERHOST) {
    m_rxCallback (this, packet, protocol, source);
  }
}

}

