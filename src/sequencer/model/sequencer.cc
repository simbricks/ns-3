/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/in.h>

#include "sequencer.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/simulator.h"

#define OUMADDR "10.1.0.255"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SequencerNetDevice");
NS_OBJECT_ENSURE_REGISTERED (SequencerNetDevice);

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
TypeId
SequencerNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SequencerNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("SequencerNetDevice")
    .AddConstructor<SequencerNetDevice> ()
    ;
    return tid;
}

SequencerNetDevice::SequencerNetDevice ()
  : m_mtu(1500), m_node(nullptr), m_rxCallback(nullptr), m_promiscRxCallback(nullptr)
{
  NS_LOG_FUNCTION_NOARGS ();
}

SequencerNetDevice::~SequencerNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SequencerNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t
SequencerNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
SequencerNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return nullptr;
}

void
SequencerNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
SequencerNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
SequencerNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
SequencerNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}

bool
SequencerNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
SequencerNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
SequencerNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
SequencerNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SequencerNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
SequencerNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address
SequencerNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool
SequencerNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

bool
SequencerNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
SequencerNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
SequencerNetDevice::SendFrom (Ptr<Packet> packet, const Address& src,
                              const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  Mac48Address dst = Mac48Address::ConvertFrom (dest);

  if (!dst.IsGroup ()) {
    // Unicast
    Ptr<NetDevice> outPort = GetLearnedState (dst);
    if (outPort != nullptr) {
      outPort->SendFrom (packet, src, dest, protocolNumber);
      return true;
    }
  }

  // Flood
  Ptr<Packet> pktCopy;
  for (auto iter = m_ports.begin (); iter != m_ports.end (); iter++) {
    pktCopy = packet->Copy ();
    Ptr<NetDevice> port = *iter;
    port->SendFrom (pktCopy, src, dest, protocolNumber);
  }

  return true;
}

Ptr<Node>
SequencerNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void
SequencerNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);
  m_node = node;
}

bool
SequencerNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
SequencerNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void
SequencerNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool
SequencerNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
SequencerNetDevice::AddSwitchPort (Ptr<NetDevice> switchPort)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (switchPort != this);
  if (!Mac48Address::IsMatchingType (switchPort->GetAddress ())) {
    NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to switch.");
  }
  if (!switchPort->SupportsSendFrom ()) {
    NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to switch.");
  }
  if (m_address == Mac48Address ()) {
    m_address = Mac48Address::ConvertFrom (switchPort->GetAddress ());
  }
  m_node->RegisterProtocolHandler (MakeCallback (&SequencerNetDevice::ReceiveFromDevice,
                                                 this),
                                   0, switchPort, true);
  m_ports.push_back (switchPort);
}

void
SequencerNetDevice::ReceiveFromDevice (Ptr<NetDevice> inPort,
                                       Ptr<const Packet> packet,
                                       uint16_t protocol,
                                       Address const &src,
                                       Address const &dst,
                                       PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();

  Mac48Address srcMac = Mac48Address::ConvertFrom (src);
  Mac48Address dstMac = Mac48Address::ConvertFrom (dst);

  if (!m_promiscRxCallback.IsNull ()) {
    m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
  }

  printf("Received packet!\n");
  switch (packetType) {
  case PACKET_HOST:
    if (dstMac == m_address) {
      Learn (srcMac, inPort);
      m_rxCallback (this, packet, protocol, src);
    }
    break;
  case PACKET_BROADCAST:
  case PACKET_MULTICAST:
    m_rxCallback (this, packet, protocol, src);
    ForwardBroadcast (inPort, packet, protocol, srcMac, dstMac);
    break;
  case PACKET_OTHERHOST:
    if (dstMac == m_address) {
      Learn (srcMac, inPort);
      m_rxCallback (this, packet, protocol, src);
    } else {
      ForwardUnicast (inPort, packet, protocol, srcMac, dstMac);
    }
    break;
  }
}

void
SequencerNetDevice::Learn (Mac48Address source, Ptr<NetDevice> inPort)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_learnState[source] = inPort;
}

Ptr<NetDevice>
SequencerNetDevice::GetLearnedState (Mac48Address source)
{
  if (m_learnState.count (source) > 0) {
    return m_learnState.at (source);
  } else {
    return nullptr;
  }
}

void
SequencerNetDevice::ForwardBroadcast (Ptr<NetDevice> inPort,
                                      Ptr<const Packet> packet,
                                      uint16_t protocol,
                                      Mac48Address src,
                                      Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();
  Learn (src, inPort);

  size_t buf_size = packet->GetSize();
  uint8_t *buffer = new uint8_t[buf_size];
  packet->CopyData(buffer, buf_size);

  if (MatchOrderedMulticast(buffer)) {
    /* OUM packet */
  } else {
    /* Regular broadcast */
    for (auto iter = m_ports.begin (); iter != m_ports.end (); iter++) {
      Ptr<NetDevice> port = *iter;
      if (port != inPort) {
        port->SendFrom (packet->Copy (), src, dst, protocol);
      }
    }
  }

  delete [] buffer;
}

void
SequencerNetDevice::ForwardUnicast (Ptr<NetDevice> inPort,
                                    Ptr<const Packet> packet,
                                    uint16_t protocol,
                                    Mac48Address src,
                                    Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();

  Learn (src, inPort);
  Ptr<NetDevice> outPort = GetLearnedState (dst);
  if (outPort != nullptr && outPort != inPort) {
      outPort->SendFrom (packet->Copy (), src, dst, protocol);
  } else {
    for (auto iter = m_ports.begin (); iter != m_ports.end (); iter++) {
      Ptr<NetDevice> port = *iter;
      if (port != inPort) {
        port->SendFrom (packet->Copy (), src, dst, protocol);
      }
    }
  }
}

bool
SequencerNetDevice::MatchOrderedMulticast (const uint8_t *pkt)
{
  // IP
  struct iphdr *iph = (struct iphdr *)pkt;
  struct sockaddr_in saddr;
  char destip[INET6_ADDRSTRLEN];
  saddr.sin_addr.s_addr = iph->daddr;
  inet_ntop(AF_INET, &(saddr.sin_addr), destip, sizeof(destip));
  if (strcmp(destip, OUMADDR)) {
    return false;
  }
  return true;
}

} // namespace ns3
