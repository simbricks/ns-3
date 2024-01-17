/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2020-2024 Max Planck Institute for Software Systems
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

#include "simbricks-trunk.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace simbricks {

extern "C" {
#include <simbricks/network/if.h>
}

NS_LOG_COMPONENT_DEFINE ("SimbricksTrunk");

TypeId SimbricksTrunk::TrunkNetDev::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::simbricks::SimbricksTrunk::TrunkNetDev")
    .SetParent<NetDevice> ()
    .SetGroupName ("SimbricksTrunk")
    ;
    return tid;
}

SimbricksTrunk::TrunkNetDev::TrunkNetDev (SimbricksTrunk& trunk_, uint8_t port_)
  : trunk(trunk_), terminated(false), port(port_), m_mtu(1500), m_node(0)
{
  NS_LOG_FUNCTION (this);
  // Nullifying callbacks explicitly is probably not needed
  m_rxCallback.Nullify ();
  m_promiscRxCallback.Nullify ();
}

SimbricksTrunk::TrunkNetDev::~TrunkNetDev()
{
  NS_LOG_FUNCTION (this);
}

void SimbricksTrunk::TrunkNetDev::ReceivedPkt (Ptr<Packet> packet)
{
  uint32_t nid = m_node->GetId ();
  Simulator::ScheduleWithContext (nid, Seconds (0.0),
      MakeEvent (&TrunkNetDev::RxInContext, this, packet));
}

void SimbricksTrunk::TrunkNetDev::RxInContext (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);

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

void SimbricksTrunk::TrunkNetDev::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}

uint32_t SimbricksTrunk::TrunkNetDev::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

Ptr<Channel> SimbricksTrunk::TrunkNetDev::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return 0;
}

void SimbricksTrunk::TrunkNetDev::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address SimbricksTrunk::TrunkNetDev::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

bool SimbricksTrunk::TrunkNetDev::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t SimbricksTrunk::TrunkNetDev::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

bool SimbricksTrunk::TrunkNetDev::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void SimbricksTrunk::TrunkNetDev::AddLinkChangeCallback (
    Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
}

bool SimbricksTrunk::TrunkNetDev::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address SimbricksTrunk::TrunkNetDev::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool SimbricksTrunk::TrunkNetDev::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address SimbricksTrunk::TrunkNetDev::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimbricksTrunk::TrunkNetDev::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool SimbricksTrunk::TrunkNetDev::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool SimbricksTrunk::TrunkNetDev::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool SimbricksTrunk::TrunkNetDev::Send (Ptr<Packet> packet,
    const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool SimbricksTrunk::TrunkNetDev::SendFrom (Ptr<Packet> packet,
    const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  if (terminated)
      return false;

  EthernetHeader header (false);
  header.SetSource (Mac48Address::ConvertFrom (source));
  header.SetDestination (Mac48Address::ConvertFrom (dest));
  header.SetLengthType (protocolNumber);

  packet->AddHeader (header);
  trunk.AdapterTx (packet, port);
  return true;
}

Ptr<Node> SimbricksTrunk::TrunkNetDev::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void SimbricksTrunk::TrunkNetDev::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}

bool SimbricksTrunk::TrunkNetDev::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void SimbricksTrunk::TrunkNetDev::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_rxCallback = cb;
}

void SimbricksTrunk::TrunkNetDev::SetPromiscReceiveCallback (
    PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_promiscRxCallback = cb;
}

bool SimbricksTrunk::TrunkNetDev::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

/******************************************************************************/

TypeId SimbricksTrunk::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::simbricks::SimbricksTrunk")
    .SetParent<Object> ()
    .SetGroupName ("SimbricksTrunk")
    .AddConstructor<SimbricksTrunk> ()
    .AddAttribute ("UnixSocket",
                   "The path to the Ethernet Unix socket",
                   StringValue ("/tmp/simbricks-eth"),
                   MakeStringAccessor (&SimbricksTrunk::m_a_uxSocketPath),
                   MakeStringChecker ())
    .AddAttribute ("ShmPath",
                   "The path to the Ethernet Unix socket",
                   StringValue (""),
                   MakeStringAccessor (&SimbricksTrunk::m_a_shmPath),
                   MakeStringChecker ())
    .AddAttribute ("SyncDelay",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksTrunk::m_a_syncDelay),                   MakeTimeChecker ())
    .AddAttribute ("PollDelay",
                   "Delay between polling for messages in non-sync mode",
                   TimeValue (NanoSeconds (100.)),
                   MakeTimeAccessor (&SimbricksTrunk::m_a_pollDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EthLatency",
                   "Max delay between outgoing messages before sync is sent",
                   TimeValue (NanoSeconds (500.)),
                   MakeTimeAccessor (&SimbricksTrunk::m_a_ethLatency),
                   MakeTimeChecker ())
    .AddAttribute ("Sync",
                   "Request synchronous interaction with device",
                   IntegerValue (1),
                   MakeIntegerAccessor (&SimbricksTrunk::m_a_sync),
                   MakeIntegerChecker<int32_t> ())
    .AddAttribute ("Listen",
                   "Use listening Unix socket instead of connecting",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimbricksTrunk::m_a_listen),
                   MakeBooleanChecker ())
    .AddAttribute ("RescheduleSyncTx",
                   "Re-schedule sync packet on packet tx",
                   BooleanValue (false),
                   MakeBooleanAccessor (
                      &SimbricksTrunk::m_a_reschedule_sync),
                   MakeBooleanChecker ())
    ;
    return tid;
}

SimbricksTrunk::SimbricksTrunk ()
  : base::GenericBaseAdapter<SimbricksProtoNetMsg, SimbricksProtoNetMsg>::
        Interface(*this),
    m_adapter(*this)
{
  NS_LOG_FUNCTION (this);
}

SimbricksTrunk::~SimbricksTrunk ()
{
  NS_LOG_FUNCTION (this);
}

void SimbricksTrunk::Start ()
{
  NS_LOG_FUNCTION (this);

  m_adapter.cfgSetSync(m_a_sync);
  m_adapter.cfgSetPollInterval (m_a_pollDelay.ToInteger(Time::PS));
  m_adapter.cfgSetRescheduleSyncTx (m_a_reschedule_sync);
  if (m_a_listen) {
    if (m_a_shmPath.empty())
      m_a_shmPath = m_a_uxSocketPath + "-shm";
    m_adapter.listen (m_a_uxSocketPath, m_a_shmPath);
  } else {
    m_adapter.connect (m_a_uxSocketPath);
  }
}

void SimbricksTrunk::Stop ()
{
  NS_LOG_FUNCTION (this);

  m_adapter.Stop ();
}

Ptr<NetDevice> SimbricksTrunk::AddNetDev()
{
  NS_LOG_FUNCTION (this);

  if (ports.size() > UINT8_MAX)
      return Ptr<NetDevice>();

  Ptr<TrunkNetDev> dev = Create<TrunkNetDev>(*this, ports.size());
  ports.push_back (dev);
  return dev;
}

size_t SimbricksTrunk::introOutPrepare(void *data, size_t maxlen)
{
  NS_LOG_FUNCTION (this);

  size_t introlen = sizeof(struct SimbricksProtoNetIntro);
  NS_ABORT_IF(introlen > maxlen);
  memset(data, 0, introlen);
  return introlen;
}

void SimbricksTrunk::introInReceived(const void *data, size_t len)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_IF(len != sizeof(struct SimbricksProtoNetIntro));
}

void SimbricksTrunk::initIfParams(SimbricksBaseIfParams &p)
{
  NS_LOG_FUNCTION (this);

  SimbricksNetIfDefaultParams(&p);
  p.link_latency = m_a_ethLatency.ToInteger(Time::PS);
  p.sync_interval = m_a_syncDelay.ToInteger(Time::PS);
  p.sync_mode = (enum SimbricksBaseIfSyncMode) m_a_sync;
}

void SimbricksTrunk::handleInMsg(volatile SimbricksProtoNetMsg *msg)
{
  NS_LOG_FUNCTION (this);

  uint8_t ty = m_adapter.inType(msg);
  NS_ABORT_MSG_IF (ty != SIMBRICKS_PROTO_NET_MSG_PACKET,
    "Unsupported msg type " << ty);

  volatile struct SimbricksProtoNetMsgPacket *pkt = &msg->packet;

  uint16_t len = pkt->len;
  Ptr<Packet> packet = Create<Packet> ((const uint8_t *) (pkt->data), len);

  NS_ABORT_MSG_IF (pkt->port >= ports.size(),
      "received packet with invalid port number: " << pkt->port);

  ports[pkt->port]->ReceivedPkt (packet);
  m_adapter.inDone(msg);
}

void SimbricksTrunk::peerTerminated()
{
  NS_LOG_FUNCTION (this);

  for (auto dev: ports) {
    dev->terminated = true;
  }
}

void SimbricksTrunk::AdapterTx (Ptr<Packet> packet, uint8_t port)
{
  volatile union SimbricksProtoNetMsg *msg_to = m_adapter.outAlloc();
  volatile struct SimbricksProtoNetMsgPacket *pkt_to = &msg_to->packet;
  pkt_to->len = packet->GetSize ();
  pkt_to->port = port;
  packet->CopyData ((uint8_t *) pkt_to->data, pkt_to->len);

  m_adapter.outSend(msg_to, SIMBRICKS_PROTO_NET_MSG_PACKET);
}

} /* namespace simbricks */
} /* namespace ns3 */

