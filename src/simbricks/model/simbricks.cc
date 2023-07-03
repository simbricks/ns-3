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

#include "simbricks.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimbricksNetDevice");

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
TypeId SimbricksNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimbricksNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("SimbricksNetDevice")
    .AddConstructor<SimbricksNetDevice> ()
    .AddAttribute ("UnixSocket",
                   "The path to the Ethernet Unix socket",
                   StringValue ("/tmp/simbricks-eth"),
                   MakeStringAccessor (&SimbricksNetDevice::m_a_uxSocketPath),
                   MakeStringChecker ())
    .AddAttribute ("SyncDelay",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksNetDevice::m_a_syncDelay),
                   MakeTimeChecker ())
    .AddAttribute ("PollDelay",
                   "Delay between polling for messages in non-sync mode",
                   TimeValue (NanoSeconds (100.)),
                   MakeTimeAccessor (&SimbricksNetDevice::m_a_pollDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EthLatency",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksNetDevice::m_a_ethLatency),
                   MakeTimeChecker ())
    .AddAttribute ("Sync",
                   "Request synchronous interaction with device",
                   IntegerValue (1),
                   MakeIntegerAccessor (&SimbricksNetDevice::m_a_sync),
                   MakeIntegerChecker<int32_t> ())
    ;
    return tid;
}

SimbricksNetDevice::SimbricksNetDevice ()
  : m_mtu(1500), m_node(0)
{
  NS_LOG_FUNCTION_NOARGS ();
  // Nullifying callbacks explicitly is probably not needed
  m_rxCallback.Nullify ();
  m_promiscRxCallback.Nullify ();
}

SimbricksNetDevice::~SimbricksNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void SimbricksNetDevice::Start ()
{
  SimbricksNetIfDefaultParams(&m_adapter.m_bifparam);
  m_adapter.m_bifparam.sock_path = m_a_uxSocketPath.c_str();

  m_adapter.m_bifparam.sync_interval = m_a_syncDelay.ToInteger (Time::PS);
  m_adapter.m_pollDelay = m_a_pollDelay;
  m_adapter.m_bifparam.link_latency = m_a_ethLatency.ToInteger (Time::PS);
  m_adapter.m_bifparam.sync_mode = (enum SimbricksBaseIfSyncMode)m_a_sync;
  m_adapter.SetReceiveCallback (
      MakeCallback (&SimbricksNetDevice::AdapterRx, this));
  m_adapter.Start ();
}

void SimbricksNetDevice::Stop ()
{
  m_adapter.Stop ();
}

void SimbricksNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t SimbricksNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel> SimbricksNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}

void SimbricksNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address SimbricksNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool SimbricksNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t SimbricksNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}

bool SimbricksNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool SimbricksNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address SimbricksNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool SimbricksNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address SimbricksNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimbricksNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool SimbricksNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool SimbricksNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool SimbricksNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool SimbricksNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
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

Ptr<Node> SimbricksNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void SimbricksNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);
  m_node = node;
}

bool SimbricksNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void SimbricksNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool SimbricksNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void SimbricksNetDevice::AdapterRx (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t nid = m_node->GetId ();
  Simulator::ScheduleWithContext (nid, Seconds (0.0),
      MakeEvent (&SimbricksNetDevice::RxInContext, this, packet));
}

void SimbricksNetDevice::RxInContext (Ptr<Packet> packet)
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

