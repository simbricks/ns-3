/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "sequencer.h"

#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SequencerNetDevice");

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
TypeId SequencerNetDevice::GetTypeId (void)
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
SequencerNetDevice::ReceiveFromDevice (Ptr<NetDevice> device,
                                       Ptr<const Packet> packet,
                                       uint16_t protocol,
                                       Address const &source,
                                       Address const &destination,
                                       PacketType packetType)
{
}

} // namespace ns3
