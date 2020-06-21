/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#pragma once

#include "ns3/net-device.h"

namespace ns3 {

class SequencerNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  SequencerNetDevice ();
  virtual ~SequencerNetDevice ();

  virtual void SetIfIndex (const uint32_t index) override;
  virtual uint32_t GetIfIndex (void) const override;

  virtual Ptr<Channel> GetChannel (void) const override;

  virtual void SetAddress (Address address) override;
  virtual Address GetAddress (void) const override;

  virtual bool SetMtu (const uint16_t mtu) override;
  virtual uint16_t GetMtu (void) const override;

  virtual bool IsLinkUp (void) const override;
  virtual void AddLinkChangeCallback (Callback<void> callback) override;

  virtual bool IsBroadcast (void) const override;
  virtual Address GetBroadcast (void) const override;
  virtual bool IsMulticast (void) const override;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const override;
  virtual Address GetMulticast (Ipv6Address addr) const override;

  virtual bool IsBridge (void) const override;
  virtual bool IsPointToPoint (void) const override;

  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber) override;

  virtual Ptr<Node> GetNode (void) const override;
  virtual void SetNode (Ptr<Node> node) override;

  virtual bool NeedsArp (void) const override;

  virtual void SetReceiveCallback (ReceiveCallback cb) override;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override;

  virtual bool SupportsSendFrom (void) const override;

private:
  uint16_t m_mtu;
  uint32_t m_ifIndex;
  Mac48Address m_address;
  Ptr<Node> m_node;
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;
};

} // namespace ns3
