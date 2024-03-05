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

#ifndef SIMBRICKS_H
#define SIMBRICKS_H

#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "simbricks-base.h"

namespace ns3 {
namespace simbricks {

extern "C" {
#include <simbricks/base/proto.h>
#include <simbricks/network/proto.h>
}

class SimbricksNetDevice :
  public NetDevice,
  public base::GenericBaseAdapter <SimbricksProtoNetMsg,
                                     SimbricksProtoNetMsg>::Interface
{
public:
  static TypeId GetTypeId ();

  SimbricksNetDevice ();
  ~SimbricksNetDevice () override;

  void Start ();
  void Stop ();

  void SetIfIndex (const uint32_t index) override;
  uint32_t GetIfIndex () const override;

  Ptr<Channel> GetChannel () const override;

  void SetAddress (Address address) override;
  Address GetAddress () const override;

  bool SetMtu (const uint16_t mtu) override;
  uint16_t GetMtu () const override;

  bool IsLinkUp () const override;
  void AddLinkChangeCallback (Callback<void> callback) override;

  bool IsBroadcast () const override;
  Address GetBroadcast () const override;
  bool IsMulticast () const override;
  Address GetMulticast (Ipv4Address multicastGroup) const override;
  Address GetMulticast (Ipv6Address addr) const override;

  bool IsBridge () const override;
  bool IsPointToPoint () const override;

  bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
  bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber) override;

  Ptr<Node> GetNode () const override;
  void SetNode (Ptr<Node> node) override;

  bool NeedsArp () const override;

  void SetReceiveCallback (ReceiveCallback cb) override;
  void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override;

  bool SupportsSendFrom () const override;

protected:
  size_t introOutPrepare(void *data, size_t maxlen) override;
  void introInReceived(const void *data, size_t len) override;
  void initIfParams(SimbricksBaseIfParams &p) override;
  void handleInMsg(volatile SimbricksProtoNetMsg *msg) override;
  void peerTerminated() override;

private:
  // SimBricks base adapter
  base::GenericBaseAdapter
      <SimbricksProtoNetMsg, SimbricksProtoNetMsg> m_adapter;

  /* params for adapter */
  std::string m_a_uxSocketPath;
  std::string m_a_shmPath;
  Time m_a_syncDelay;
  Time m_a_pollDelay;
  Time m_a_ethLatency;
  int m_a_sync;
  bool m_a_listen;
  bool m_a_reschedule_sync;


  uint16_t m_mtu;
  uint32_t m_ifIndex;
  /* this is unused: but necessary for ns-3 */
  Mac48Address m_address;
  Ptr<Node> m_node;
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  bool terminated;

  void AdapterRx (Ptr<Packet> packet);
  void RxInContext (Ptr<Packet> packet);
};

} /* namespace simbricks */
} /* namespace ns3 */

#endif /* SIMBRICKS_H */

