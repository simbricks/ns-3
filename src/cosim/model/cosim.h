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

#ifndef COSIM_H
#define COSIM_H

#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "cosim-adapter.h"

namespace ns3 {

/* ... */

class CosimNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  CosimNetDevice ();
  virtual ~CosimNetDevice ();

  void Start ();
  void Stop ();

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
  CosimAdapter m_adapter;

  /* params for adapter */
  std::string m_a_uxSocketPath;
  Time m_a_syncDelay;
  Time m_a_pollDelay;
  Time m_a_ethLatency;
  bool m_a_sync;


  uint16_t m_mtu;
  uint32_t m_ifIndex;
  /* this is unused: but necessary for ns-3 */
  Mac48Address m_address;
  Ptr<Node> m_node;
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  void AdapterRx (Ptr<Packet> packet);
  void RxInContext (Ptr<Packet> packet);
};



}

#endif /* COSIM_H */

