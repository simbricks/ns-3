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

#include "cosim-adapter.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"


namespace ns3 {

extern "C" {
#include <netsim.h>
#include <cosim_eth_proto.h>
}

NS_LOG_COMPONENT_DEFINE ("CosimAdapter");

CosimAdapter::CosimAdapter ()
{
  m_nsif = new netsim_interface;
}

CosimAdapter::~CosimAdapter ()
{
  delete m_nsif;
}

void CosimAdapter::Start ()
{
  int sync = m_sync;
  int ret;

  NS_ABORT_MSG_IF (m_uxSocketPath.empty(), "CosimAdapter::Connect: unix socket"
          " path empty");

  ret = netsim_init(m_nsif, m_uxSocketPath.c_str(), &sync);

  NS_ABORT_MSG_IF (ret != 0, "CosimAdapter::Connect: netsim_init failed");
  NS_ABORT_MSG_IF (m_sync && !sync,
          "CosimAdapter::Connect: request for sync failed");

  if (m_sync)
    m_syncTxEvent = Simulator::Schedule (m_syncDelay, &CosimAdapter::SendSyncEvent, this);

  m_pollEvent = Simulator::ScheduleNow (&CosimAdapter::PollEvent, this);
}

void CosimAdapter::Stop ()
{
  if (m_sync)
    Simulator::Cancel (m_syncTxEvent);
  Simulator::Cancel (m_pollEvent);
}

void CosimAdapter::SetReceiveCallback (RxCallback cb)
{
  m_rxCallback = cb;
}

bool CosimAdapter::Transmit (Ptr<const Packet> packet)
{
  volatile union cosim_eth_proto_n2d *msg;
  volatile struct cosim_eth_proto_n2d_recv *recv;

  NS_ABORT_MSG_IF (packet->GetSize () > 2048,
          "CosimAdapter::Transmit: packet too large");

  msg = AllocTx ();
  recv = &msg->recv;
  recv->len = packet->GetSize ();
  recv->port = 0;
  packet->CopyData ((uint8_t *) recv->data, recv->len);

  recv->own_type = COSIM_ETH_PROTO_N2D_MSG_RECV | COSIM_ETH_PROTO_N2D_OWN_DEV;

  if (m_sync) {
    Simulator::Cancel (m_syncTxEvent);
    m_syncTxEvent = Simulator::Schedule (m_syncDelay,
            &CosimAdapter::SendSyncEvent, this);
  }

  return true;
}

void CosimAdapter::ReceivedPacket (const void *buf, size_t len)
{
    Ptr<Packet> packet = Create<Packet> (
            reinterpret_cast<const uint8_t *> (buf), len);
    m_rxCallback (packet);
}

volatile union cosim_eth_proto_n2d *CosimAdapter::AllocTx ()
{
  volatile union cosim_eth_proto_n2d *msg;
  msg = netsim_n2d_alloc (m_nsif, Simulator::Now ().ToInteger (Time::PS),
          m_ethLatency.ToInteger (Time::PS));
  NS_ABORT_MSG_IF (msg == NULL,
          "CosimAdapter::AllocTx: netsim_n2d_alloc failed");
  return msg;
}

bool CosimAdapter::Poll ()
{
  volatile union cosim_eth_proto_d2n *msg;
  uint8_t ty;

  msg = netsim_d2n_poll (m_nsif, Simulator::Now ().ToInteger (Time::PS));
  m_nextTime = PicoSeconds (netsim_d2n_timestamp (m_nsif));

  if (!msg)
    return false;

  ty = msg->dummy.own_type & COSIM_ETH_PROTO_D2N_MSG_MASK;
  switch (ty) {
    case COSIM_ETH_PROTO_D2N_MSG_SEND:
      ReceivedPacket ((const void *) msg->send.data, msg->send.len);
      break;

    case COSIM_ETH_PROTO_D2N_MSG_SYNC:
      break;

    default:
      NS_ABORT_MSG ("CosimAdapter::Poll: unsupported message type " << ty);
  }

  netsim_d2n_done (m_nsif, msg);
  return true;
}

void CosimAdapter::PollEvent ()
{
  while (Poll ());

  if (m_sync){
    while (m_nextTime <= Simulator::Now ())
      Poll ();

    m_pollEvent = Simulator::Schedule (m_nextTime -  Simulator::Now (),
            &CosimAdapter::PollEvent, this);
  } else {
    m_pollEvent = Simulator::Schedule (m_pollDelay,
            &CosimAdapter::PollEvent, this);

  }
}

void CosimAdapter::SendSyncEvent ()
{
  volatile union cosim_eth_proto_n2d *msg = AllocTx ();

  msg->sync.own_type = COSIM_ETH_PROTO_N2D_MSG_SYNC | COSIM_ETH_PROTO_N2D_OWN_DEV;

  m_syncTxEvent = Simulator::Schedule (m_syncDelay, &CosimAdapter::SendSyncEvent, this);
}

}
