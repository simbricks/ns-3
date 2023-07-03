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
#include "simbricks-adapter-nicif.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"


namespace ns3 {
#include <simbricks/base/cxxatomicfix.h>
extern "C" {
#include <simbricks/nicif/nicif.h>
#include <simbricks/network/proto.h>
}

NS_LOG_COMPONENT_DEFINE ("SimbricksAdapterNicIf");

SimbricksAdapterNicIf::SimbricksAdapterNicIf ()
{
  m_nsif = new SimbricksNicIf;
}

SimbricksAdapterNicIf::~SimbricksAdapterNicIf ()
{
  delete m_nsif;
}

void SimbricksAdapterNicIf::Start ()
{
  int sync = m_bifparam.sync_mode;
  int ret;

  NS_ABORT_MSG_IF (m_bifparam.sock_path == NULL, "SimbricksAdapter::Connect: unix socket"
          " path empty");
  NS_ABORT_MSG_IF (m_shmPath.c_str() == NULL, "SimbricksAdapter::Connect: shared memory"
          " path empty");
  
  ret = SimbricksNicIfInit(m_nsif, m_shmPath.c_str(), &m_bifparam, nullptr, nullptr);

  NS_ABORT_MSG_IF (ret != 0, "SimbricksAdapter::SimbricksNicIfInit failed");
  NS_ABORT_MSG_IF (m_bifparam.sync_mode && !sync,
          "SimbricksAdapter::Connect: request for sync failed");

  if (m_bifparam.sync_mode)
    m_syncTxEvent = Simulator::ScheduleNow (&SimbricksAdapterNicIf::SendSyncEvent, this);

  m_pollEvent = Simulator::ScheduleNow (&SimbricksAdapterNicIf::PollEvent, this);
}

void SimbricksAdapterNicIf::Stop ()
{
  if (m_bifparam.sync_mode)
    Simulator::Cancel (m_syncTxEvent);
  Simulator::Cancel (m_pollEvent);
}

void SimbricksAdapterNicIf::SetReceiveCallback (RxCallback cb)
{
  m_rxCallback = cb;
}

bool SimbricksAdapterNicIf::Transmit (Ptr<const Packet> packet)
{
  volatile union SimbricksProtoNetMsg *msg;
  volatile struct SimbricksProtoNetMsgPacket *recv;

  /*NS_ABORT_MSG_IF (packet->GetSize () > 2048,
          "SimbricksAdapter::Transmit: packet too large");*/

  msg = AllocTx ();
  recv = &msg->packet;
  recv->len = packet->GetSize ();
  recv->port = 0;
  packet->CopyData ((uint8_t *) recv->data, recv->len);


  SimbricksNetIfOutSend(&m_nsif->net, msg, SIMBRICKS_PROTO_NET_MSG_PACKET);

  if (m_bifparam.sync_mode) {
    Simulator::Cancel (m_syncTxEvent);
    m_syncTxEvent = Simulator::Schedule(PicoSeconds(m_bifparam.sync_interval),
            &SimbricksAdapterNicIf::SendSyncEvent, this);
  }

  return true;
}

void SimbricksAdapterNicIf::ReceivedPacket (const void *buf, size_t len)
{
    Ptr<Packet> packet = Create<Packet> (
            reinterpret_cast<const uint8_t *> (buf), len);
    m_rxCallback (packet);
}

volatile union SimbricksProtoNetMsg *SimbricksAdapterNicIf::AllocTx ()
{
  volatile union SimbricksProtoNetMsg *msg;
  msg = SimbricksNetIfOutAlloc (&m_nsif->net, Simulator::Now ().ToInteger (Time::PS));
  NS_ABORT_MSG_IF (msg == NULL,
          "SimbricksAdapter::AllocTx: SimbricksNicIfD2NAlloc failed");
  return msg;
}

bool SimbricksAdapterNicIf::Poll ()
{
  volatile union SimbricksProtoNetMsg *msg;
  uint8_t ty;

  msg = SimbricksNetIfInPoll (&m_nsif->net, Simulator::Now ().ToInteger (Time::PS));
  m_nextTime = PicoSeconds(SimbricksNetIfInTimestamp (&m_nsif->net));

  if (!msg)
    return false;

  ty = SimbricksNetIfInType(&m_nsif->net, msg);
  switch (ty) {
    case SIMBRICKS_PROTO_NET_MSG_PACKET:
      ReceivedPacket ((const void *) msg->packet.data, msg->packet.len);
      break;

    case SIMBRICKS_PROTO_MSG_TYPE_SYNC:
      break;

    default:
      NS_ABORT_MSG ("SimbricksAdapter::Poll: unsupported message type " << ty);
  }

  SimbricksNetIfInDone (&m_nsif->net, msg);
  return true;
}

void SimbricksAdapterNicIf::PollEvent ()
{
  while (Poll ());

  if (m_bifparam.sync_mode){
    while (m_nextTime <= Simulator::Now ())
      Poll ();

    m_pollEvent = Simulator::Schedule (m_nextTime -  Simulator::Now (),
            &SimbricksAdapterNicIf::PollEvent, this);
  } else {
    m_pollEvent = Simulator::Schedule (m_pollDelay,
            &SimbricksAdapterNicIf::PollEvent, this);

  }
}

void SimbricksAdapterNicIf::SendSyncEvent ()
{
  volatile union SimbricksProtoNetMsg *msg = AllocTx ();
  NS_ABORT_MSG_IF (msg == NULL,
          "SimbricksAdapter::AllocTx: SimbricksNetIfOutAlloc failed");
  SimbricksBaseIfOutSend(&m_nsif->net.base, &msg->base, SIMBRICKS_PROTO_MSG_TYPE_SYNC);

  m_syncTxEvent = Simulator::Schedule (PicoSeconds (m_bifparam.sync_interval), &SimbricksAdapterNicIf::SendSyncEvent, this);
}

}
