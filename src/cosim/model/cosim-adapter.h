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

#ifndef COSIM_ADAPTER_H
#define COSIM_ADAPTER_H

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/event-id.h"

namespace ns3 {

struct netsim_interface;
union cosim_eth_proto_n2d;

class CosimAdapter
{
public:
  std::string m_uxSocketPath;
  Time m_syncDelay;
  Time m_pollDelay;
  Time m_ethLatency;
  bool m_sync;

  CosimAdapter ();
  ~CosimAdapter ();

  void Start ();
  void Stop ();

  typedef Callback<void, Ptr<Packet>> RxCallback;

  void SetReceiveCallback (RxCallback cb);
  bool Transmit (Ptr<const Packet> packet);

private:
  struct netsim_interface *m_nsif;
  bool m_isConnected;
  RxCallback m_rxCallback;
  Time m_nextTime;
  EventId m_syncTxEvent;
  EventId m_pollEvent;

  void ReceivedPacket (const void *buf, size_t len);
  volatile union cosim_eth_proto_n2d *AllocTx ();
  bool Poll ();
  void PollEvent ();
  void SendSyncEvent ();

};

}

#endif /* COSIM_ADAPTER_H */
