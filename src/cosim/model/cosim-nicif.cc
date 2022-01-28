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

#include "cosim-nicif.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/integer.h"
#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CosimNetDeviceNicIf");

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
TypeId CosimNetDeviceNicIf::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CosimNetDeviceNicIf")
    .SetParent<NetDevice> ()
    .SetGroupName ("CosimNetDeviceNicIf")
    .AddConstructor<CosimNetDeviceNicIf> ()
    .AddAttribute ("UnixSocket",
                   "The path to the Ethernet Unix socket",
                   StringValue ("/tmp/cosim-eth"),
                   MakeStringAccessor (&CosimNetDeviceNicIf::m_a_uxSocketPath),
                   MakeStringChecker ())
    .AddAttribute ("Shm",
                   "The path to the shared memory",
                   StringValue ("/tmp/cosim-shm"),
                   MakeStringAccessor (&CosimNetDeviceNicIf::m_a_shmPath),
                   MakeStringChecker ())
    .AddAttribute ("SyncDelay",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (100.)),
                   MakeTimeAccessor (&CosimNetDeviceNicIf::m_a_syncDelay),
                   MakeTimeChecker ())
    .AddAttribute ("PollDelay",
                   "Delay between polling for messages in non-sync mode",
                   TimeValue (NanoSeconds (100.)),
                   MakeTimeAccessor (&CosimNetDeviceNicIf::m_a_pollDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EthLatency",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&CosimNetDeviceNicIf::m_a_ethLatency),
                   MakeTimeChecker ())
    .AddAttribute ("Sync",
                   "Request synchronous interaction with device",
                   BooleanValue (true),
                   MakeBooleanAccessor (&CosimNetDeviceNicIf::m_a_sync),
                   MakeBooleanChecker ())
    .AddAttribute ("SyncMode",
                   "Set synchronous mode",
                   BooleanValue (false),
                   MakeBooleanAccessor (&CosimNetDeviceNicIf::m_a_sync_mode),
                   MakeBooleanChecker ())
    ;
    return tid;
}

CosimNetDeviceNicIf::CosimNetDeviceNicIf ()
  : m_mtu(1500), m_node(0), m_rxCallback(0), m_promiscRxCallback(0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

CosimNetDeviceNicIf::~CosimNetDeviceNicIf ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void CosimNetDeviceNicIf::Start ()
{
  
  m_adapter.m_uxSocketPath = m_a_uxSocketPath;
  m_adapter.m_shmPath = m_a_shmPath;
  m_adapter.m_syncDelay = m_a_syncDelay;
  m_adapter.m_pollDelay = m_a_pollDelay;
  m_adapter.m_ethLatency = m_a_ethLatency;
  m_adapter.m_sync = m_a_sync;
  m_adapter.m_sync_mode = m_a_sync_mode;
  m_adapter.SetReceiveCallback (
      MakeCallback (&CosimNetDeviceNicIf::AdapterRx, this));
  m_adapter.Start ();
}

void CosimNetDeviceNicIf::Stop ()
{
  m_adapter.Stop ();
}

void CosimNetDeviceNicIf::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t CosimNetDeviceNicIf::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel> CosimNetDeviceNicIf::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}

void CosimNetDeviceNicIf::SetAddress (Address address)
{
  NS_LOG_FUNCTION (address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address CosimNetDeviceNicIf::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool CosimNetDeviceNicIf::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t CosimNetDeviceNicIf::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}

bool CosimNetDeviceNicIf::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void CosimNetDeviceNicIf::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool CosimNetDeviceNicIf::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address CosimNetDeviceNicIf::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool CosimNetDeviceNicIf::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address CosimNetDeviceNicIf::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address CosimNetDeviceNicIf::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool CosimNetDeviceNicIf::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool CosimNetDeviceNicIf::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool CosimNetDeviceNicIf::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool CosimNetDeviceNicIf::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
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

Ptr<Node> CosimNetDeviceNicIf::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void CosimNetDeviceNicIf::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);
  m_node = node;
}

bool CosimNetDeviceNicIf::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void CosimNetDeviceNicIf::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void CosimNetDeviceNicIf::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool CosimNetDeviceNicIf::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void CosimNetDeviceNicIf::AdapterRx (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t nid = m_node->GetId ();
  Simulator::ScheduleWithContext (nid, Seconds (0.0),
      MakeEvent (&CosimNetDeviceNicIf::RxInContext, this, packet));
}

void CosimNetDeviceNicIf::RxInContext (Ptr<Packet> packet)
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

