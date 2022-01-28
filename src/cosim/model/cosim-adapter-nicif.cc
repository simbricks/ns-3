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

#include <cassert>
#include "cosim-adapter-nicif.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"

#define D2N_ELEN (1536 + 64)
#define D2N_ENUM 8192

#define N2D_ELEN (1536 + 64)
#define N2D_ENUM 8192

namespace ns3 {

extern "C" {
#include <simbricks/nicif/nicif.h>
#include <simbricks/proto/network.h>

}

NS_LOG_COMPONENT_DEFINE ("CosimAdapterNicIf");

CosimAdapterNicIf::CosimAdapterNicIf ()
{
  m_nsif = new SimbricksNicIf;
}

CosimAdapterNicIf::~CosimAdapterNicIf ()
{
  delete m_nsif;
}

void CosimAdapterNicIf::Start ()
{
  int sync = m_sync;
  int ret;

  NS_ABORT_MSG_IF (m_uxSocketPath.empty(), "CosimAdapter::Connect: unix socket"
          " path empty");
  NS_ABORT_MSG_IF (m_shmPath.empty(), "CosimAdapter::Connect: shared memory"
          " path empty");

  nsparams.sync_pci = 1;
  nsparams.sync_eth = 1;
  nsparams.pci_socket_path = nullptr;
  nsparams.eth_socket_path = m_uxSocketPath.c_str();
  nsparams.shm_path = m_shmPath.c_str();
  nsparams.pci_latency = 0;
  nsparams.eth_latency = m_ethLatency.GetPicoSeconds();
  nsparams.sync_delay = m_syncDelay.GetPicoSeconds();
  assert(m_sync_mode == SIMBRICKS_PROTO_SYNC_SIMBRICKS ||
         m_sync_mode == SIMBRICKS_PROTO_SYNC_BARRIER);
  nsparams.sync_mode = m_sync_mode;

  NS_LOG_INFO ("before nicif init\n");
  
  ret = SimbricksNicIfInit(m_nsif, &nsparams, nullptr);

  NS_ABORT_MSG_IF (ret != 0, "CosimAdapter::Connect: SimbricksNicIfInit failed");
  NS_ABORT_MSG_IF (m_sync && !sync,
          "CosimAdapter::Connect: request for sync failed");

  NS_LOG_INFO ("sync_eth = " << nsparams.sync_eth << "\n");



  if (m_sync)
    m_syncTxEvent = Simulator::ScheduleNow (&CosimAdapterNicIf::SendSyncEvent, this);

  m_pollEvent = Simulator::ScheduleNow (&CosimAdapterNicIf::PollEvent, this);
}

void CosimAdapterNicIf::Stop ()
{
  if (m_sync)
    Simulator::Cancel (m_syncTxEvent);
  Simulator::Cancel (m_pollEvent);
}

void CosimAdapterNicIf::SetReceiveCallback (RxCallback cb)
{
  m_rxCallback = cb;
}

bool CosimAdapterNicIf::Transmit (Ptr<const Packet> packet)
{
  volatile union SimbricksProtoNetD2N *msg;
  volatile struct SimbricksProtoNetD2NSend *send;

  /*NS_ABORT_MSG_IF (packet->GetSize () > 2048,
          "CosimAdapter::Transmit: packet too large");*/

  msg = AllocTx ();
  send = &msg->send;
  send->len = packet->GetSize ();
  send->port = 0;
  packet->CopyData ((uint8_t *) send->data, send->len);

  send->own_type = SIMBRICKS_PROTO_NET_D2N_MSG_SEND | SIMBRICKS_PROTO_NET_D2N_OWN_NET;

  if (m_sync) {
    Simulator::Cancel (m_syncTxEvent);
    m_syncTxEvent = Simulator::Schedule (m_syncDelay,
            &CosimAdapterNicIf::SendSyncEvent, this);
  }

  return true;
}

void CosimAdapterNicIf::ReceivedPacket (const void *buf, size_t len)
{
    Ptr<Packet> packet = Create<Packet> (
            reinterpret_cast<const uint8_t *> (buf), len);
    m_rxCallback (packet);
}

volatile union SimbricksProtoNetD2N *CosimAdapterNicIf::AllocTx ()
{
  volatile union SimbricksProtoNetD2N *msg;
  msg = SimbricksNicIfD2NAlloc (m_nsif, Simulator::Now ().ToInteger (Time::PS));
  NS_ABORT_MSG_IF (msg == NULL,
          "CosimAdapter::AllocTx: SimbricksNicIfD2NAlloc failed");
  return msg;
}


void CosimAdapterNicIf::SimbricksNicIfN2DDone(struct SimbricksNicIf *nicif,
                           volatile union SimbricksProtoNetN2D *msg) {
  msg->dummy.own_type =
      (msg->dummy.own_type & SIMBRICKS_PROTO_NET_N2D_MSG_MASK) |
      SIMBRICKS_PROTO_NET_N2D_OWN_NET;
}

void CosimAdapterNicIf::SimbricksNicIfN2DNext(struct SimbricksNicIf *nicif) {
  nicif->n2d_pos = (nicif->n2d_pos + 1) % N2D_ENUM;
}




bool CosimAdapterNicIf::Poll ()
{
  volatile union SimbricksProtoNetN2D *msg;
  uint8_t ty;

  msg = SimbricksNicIfN2DPoll (m_nsif, Simulator::Now ().ToInteger (Time::PS));
  m_nextTime = PicoSeconds(m_nsif->eth_last_rx_time);

  if (!msg)
    return false;

  ty = msg->dummy.own_type & SIMBRICKS_PROTO_NET_N2D_MSG_MASK;
  switch (ty) {
    case SIMBRICKS_PROTO_NET_N2D_MSG_RECV:
      ReceivedPacket ((const void *) msg->recv.data, msg->recv.len);
      break;

    case SIMBRICKS_PROTO_NET_N2D_MSG_SYNC:
      break;

    default:
      NS_ABORT_MSG ("CosimAdapter::Poll: unsupported message type " << ty);
  }

  SimbricksNicIfN2DDone(m_nsif, msg);
  SimbricksNicIfN2DNext(m_nsif);
  return true;
}

void CosimAdapterNicIf::PollEvent ()
{
  while (Poll ());

  if (m_sync){
    while (m_nextTime <= Simulator::Now ())
      Poll ();

    m_pollEvent = Simulator::Schedule (m_nextTime -  Simulator::Now (),
            &CosimAdapterNicIf::PollEvent, this);
  } else {
    m_pollEvent = Simulator::Schedule (m_pollDelay,
            &CosimAdapterNicIf::PollEvent, this);

  }
}

void CosimAdapterNicIf::SendSyncEvent ()
{
  volatile union SimbricksProtoNetD2N *msg = AllocTx ();

  msg->sync.own_type = SIMBRICKS_PROTO_NET_D2N_MSG_SYNC |
      SIMBRICKS_PROTO_NET_D2N_OWN_NET;

  m_syncTxEvent = Simulator::Schedule (m_syncDelay, &CosimAdapterNicIf::SendSyncEvent, this);
}

}
