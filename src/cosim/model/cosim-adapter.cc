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

#include <simbricks/base/cxxatomicfix.h>
extern "C" {
#include <simbricks/network/if.h>
#include <simbricks/network/proto.h>
}

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CosimAdapter");

CosimAdapter::CosimAdapter ()
{
  m_nsif = new SimbricksNetIf;
}

CosimAdapter::~CosimAdapter ()
{
  delete m_nsif;
}

void CosimAdapter::Start ()
{
  int sync = m_bifparam.sync_mode;
  int ret;

  NS_ABORT_MSG_IF (m_bifparam.sock_path == NULL, "SimbricksAdapter::Connect: unix socket"
          " path empty");

  ret = SimbricksNetIfInit(m_nsif, &m_bifparam, m_bifparam.sock_path, &sync);

  NS_ABORT_MSG_IF (ret != 0, "SimbricksAdapter::Connect: SimbricksNetIfInit failed");
  NS_ABORT_MSG_IF (m_bifparam.sync_mode && !sync,
          "SimbricksAdapter::Connect: request for sync failed");

  if (m_bifparam.sync_mode)
    m_syncTxEvent = Simulator::ScheduleNow (&CosimAdapter::SendSyncEvent, this);

  m_pollEvent = Simulator::ScheduleNow (&CosimAdapter::PollEvent, this);
}

void CosimAdapter::Stop ()
{
  if (m_bifparam.sync_mode)
    Simulator::Cancel (m_syncTxEvent);
  Simulator::Cancel (m_pollEvent);
}

void CosimAdapter::SetReceiveCallback (RxCallback cb)
{
  m_rxCallback = cb;
}

bool CosimAdapter::Transmit (Ptr<const Packet> packet)
{
  volatile union SimbricksProtoNetMsg *msg;
  volatile struct SimbricksProtoNetMsgPacket *recv;

  /*NS_ABORT_MSG_IF (packet->GetSize () > 2048,
          "CosimAdapter::Transmit: packet too large");*/

  msg = AllocTx ();
  recv = &msg->packet;
  recv->len = packet->GetSize ();
  recv->port = 0;
  packet->CopyData ((uint8_t *) recv->data, recv->len);

  SimbricksNetIfOutSend(m_nsif, msg, SIMBRICKS_PROTO_NET_MSG_PACKET);

  if (m_bifparam.sync_mode) {
    Simulator::Cancel (m_syncTxEvent);
    m_syncTxEvent = Simulator::Schedule ( PicoSeconds (m_bifparam.sync_interval),
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

volatile union SimbricksProtoNetMsg *CosimAdapter::AllocTx ()
{
  volatile union SimbricksProtoNetMsg *msg;
  do {
    msg = SimbricksNetIfOutAlloc (m_nsif, Simulator::Now ().ToInteger (Time::PS));
  } while (!msg);

  //TODO: fix it to wait until alloc success
  NS_ABORT_MSG_IF (msg == NULL,
          "SimbricksAdapter::AllocTx: SimbricksNetIfOutAlloc failed");
  return msg;
}

bool CosimAdapter::Poll ()
{
  volatile union SimbricksProtoNetMsg *msg;
  uint8_t ty;

  msg = SimbricksNetIfInPoll (m_nsif, Simulator::Now ().ToInteger (Time::PS));
  m_nextTime = PicoSeconds (SimbricksNetIfInTimestamp (m_nsif));
  
  if (!msg)
    return false;

  ty = SimbricksNetIfInType(m_nsif, msg);
  switch (ty) {
    case SIMBRICKS_PROTO_NET_MSG_PACKET:
      ReceivedPacket ((const void *) msg->packet.data, msg->packet.len);
      break;

    case SIMBRICKS_PROTO_MSG_TYPE_SYNC:
      break;

    default:
      NS_ABORT_MSG ("CosimAdapter::Poll: unsupported message type " << ty);
  }

  SimbricksNetIfInDone (m_nsif, msg);
  return true;
}

void CosimAdapter::PollEvent ()
{
  while (Poll ());

  if (m_bifparam.sync_mode){
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
  volatile union SimbricksProtoNetMsg *msg = AllocTx ();
  NS_ABORT_MSG_IF (msg == NULL,
          "SimbricksAdapter::AllocTx: SimbricksNetIfOutAlloc failed");

  // msg->sync.own_type = SIMBRICKS_PROTO_NET_N2D_MSG_SYNC |
  //     SIMBRICKS_PROTO_NET_N2D_OWN_DEV;
  SimbricksBaseIfOutSend(&m_nsif->base, &msg->base, SIMBRICKS_PROTO_MSG_TYPE_SYNC);

  m_syncTxEvent = Simulator::Schedule ( PicoSeconds (m_bifparam.sync_interval), &CosimAdapter::SendSyncEvent, this);
}

}
