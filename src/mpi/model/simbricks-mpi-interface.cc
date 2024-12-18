/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George Riley <riley@ece.gatech.edu>
 *
 */

// This object contains static methods that provide an easy interface
// to the necessary MPI information.

#include <iostream>
#include <iomanip>
#include <list>

#include "simbricks-mpi-interface.h"
#include "mpi-interface.h"

#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/net-device.h"
#include "ns3/simulator.h"
#include "ns3/simulator-impl.h"
#include "ns3/nstime.h"
#include "ns3/log.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimbricksMpiInterface");

uint32_t SimbricksMpiInterface::m_sid;
bool SimbricksMpiInterface::m_initialized;
bool SimbricksMpiInterface::m_enabled;
uint32_t SimbricksMpiInterface::m_size;
std::map<uint32_t, SimbricksNetIf*> SimbricksMpiInterface::m_nsif;
std::map<uint32_t, bool> SimbricksMpiInterface:: m_isConnected;
std::map<uint32_t, uint64_t> SimbricksMpiInterface::m_nextTime;
std::map<uint32_t, EventId> SimbricksMpiInterface::m_syncTxEvent;
std::map<uint32_t, EventId> SimbricksMpiInterface::m_pollEvent;
bool SimbricksMpiInterface::m_syncMode = true;
std::map<uint32_t, std::map<uint32_t,uint64_t>> SimbricksMpiInterface::conns;
std::map<uint64_t, std::vector<uint32_t>> SimbricksMpiInterface::connsRev;
std::map<uint32_t, SimbricksBaseIfParams*> SimbricksMpiInterface::m_bifparam;
std::map<uint32_t, Time> SimbricksMpiInterface::m_pollDelay;
std::string SimbricksMpiInterface::m_dir;
unsigned SimbricksMpiInterface::n_bifs = 0;
struct SimBricksBaseIfEstablishData* SimbricksMpiInterface::ests;
std::map<uint32_t, SimbricksProtoNetIntro*> SimbricksMpiInterface::m_net_intro;
std::map<uint32_t, SimbricksBaseIfSHMPool*> SimbricksMpiInterface::m_pool;

TypeId 
SimbricksMpiInterface::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimbricksMpiInterface")
    .SetParent<Object> ()
    .SetGroupName ("Mpi")
  ;
  return tid;
}

void
SimbricksMpiInterface::Destroy ()
{
  NS_LOG_FUNCTION (this);
  for(auto i = m_bifparam.begin(); i != m_bifparam.end(); i++) delete i->second;
  // for(auto i = m_nsif.begin(); i != m_nsif.end(); i++){
  //   Simulator::Cancel (m_syncTxEvent[i->first]);
  //   // Simulator::Cancel (m_pollEvent[i->first]);
  // }
  for(auto i = m_nsif.begin(); i != m_nsif.end(); i++) delete i->second;
}

uint32_t
SimbricksMpiInterface::GetSystemId ()
{
  if (!m_initialized)
    {
      Simulator::GetImplementation ();
      m_initialized = true;
    }
  return m_sid;
}

uint32_t
SimbricksMpiInterface::GetSize ()
{
  if (!m_initialized)
    {
      Simulator::GetImplementation ();
      m_initialized = true;
    }
  return m_size;
}

bool
SimbricksMpiInterface::IsEnabled ()
{
  if (!m_initialized)
    {
      Simulator::GetImplementation ();
      m_initialized = true;
    }
  return m_enabled;
}

void
SimbricksMpiInterface::Enable (int* pargc, char*** pargv)
{
  NS_LOG_FUNCTION (this << pargc << pargv);
  m_enabled = true;
  m_initialized = true;
  m_sid = atoi((*pargv)[1]);
  m_size = atoi((*pargv)[2]);
  m_dir = (*pargv)[3];
  // std::cout << "Sid is " << m_sid << std::flush;
  // assert(0);
}

volatile union SimbricksProtoNetMsg* SimbricksMpiInterface::AllocTx (int systemId)
{
  volatile union SimbricksProtoNetMsg *msg;
  do {
    msg = SimbricksNetIfOutAlloc (m_nsif[systemId], Simulator::Now ().ToInteger (Time::PS));
  } while (!msg);

  //TODO: fix it to wait until alloc success
  NS_ABORT_MSG_IF (msg == NULL,
          "SimbricksAdapter::AllocTx: SimbricksNetIfOutAlloc failed");
  return msg;
}

void
SimbricksMpiInterface::SendPacket (Ptr<Packet> packet, const Time& rxTime, uint32_t node, uint32_t dev)
{
  NS_LOG_FUNCTION (this << packet << rxTime.GetTimeStep () << node << dev);
  volatile union SimbricksProtoNetMsg *msg;
  volatile struct SimbricksProtoNetMsgPacket *recv;

  /*NS_ABORT_MSG_IF (packet->GetSize () > 2048,
          "SimbricksMpiInterface::Transmit: packet too large");*/

  Ptr<Node> nd = NodeList::GetNode(node);
  int systemId = nd->GetSystemId();
  msg = AllocTx (systemId);
  recv = &msg->packet;

  uint64_t t = rxTime.GetInteger ();
  uint64_t* pTime = (uint64_t *)(recv->data);
  *pTime++ = t;
  uint32_t* pData = reinterpret_cast<uint32_t *> (pTime);
  *pData++ = node;
  *pData++ = dev;

  recv->len = packet->GetSize ()+8+4+4;
  recv->port = 0;
  packet->CopyData (reinterpret_cast<uint8_t *> (pData), packet->GetSize ());

  SimbricksNetIfOutSend(m_nsif[systemId], msg, SIMBRICKS_PROTO_NET_MSG_PACKET);

  // if (m_syncMode) {
  //   Simulator::Cancel (m_syncTxEvent[systemId]);
  //   m_syncTxEvent[systemId] = Simulator::Schedule ( PicoSeconds (m_bifparam[systemId]->sync_interval),
  //           &SimbricksMpiInterface::SendSyncEvent, this, systemId);
  // }
}

void SimbricksMpiInterface::ReceivedPacket (const void *buf, size_t len, uint64_t time)
{ 
    uint64_t* pTime = (uint64_t *)(buf);
    uint64_t tim = *pTime++;
    uint32_t* pData = reinterpret_cast<uint32_t *> (pTime);
    uint32_t node = *pData++;
    uint32_t dev  = *pData++;

    Time rxTime (tim);

    int count = sizeof (time) + sizeof (node) + sizeof (dev);
    Ptr<Packet> packet = Create<Packet> (reinterpret_cast<uint8_t *> (pData), len - 16);
    // Find the correct node/device to schedule receive event
    Ptr<Node> pNode = NodeList::GetNode (node);
    Ptr<MpiReceiver> pMpiRec = 0;
    uint32_t nDevices = pNode->GetNDevices ();
    for (uint32_t i = 0; i < nDevices; ++i)
      {
        Ptr<NetDevice> pThisDev = pNode->GetDevice (i);
        if (pThisDev->GetIfIndex () == dev)
          {
            pMpiRec = pThisDev->GetObject<MpiReceiver> ();
            break;
          }
      }

    NS_ASSERT (pNode && pMpiRec);

    // Schedule the rx event
    Simulator::ScheduleWithContext (pNode->GetId (), rxTime - Simulator::Now (),
                                    &MpiReceiver::Receive, pMpiRec, packet);
}

void SimbricksMpiInterface::SendSyncEvent (uint64_t delay)
{

  for(int i=0; i<connsRev[delay].size(); i++){
    // std::cout << "\n" << connsRev[delay].size() << " " << delay;
    int systemId = connsRev[delay][i];
    volatile union SimbricksProtoNetMsg *msg = AllocTx (systemId);
    NS_ABORT_MSG_IF (msg == NULL,
            "SimbricksAdapter::AllocTx: SimbricksNetIfOutAlloc failed");

    // msg->sync.own_type = SIMBRICKS_PROTO_NET_N2D_MSG_SYNC |
    //     SIMBRICKS_PROTO_NET_N2D_OWN_DEV;
    SimbricksBaseIfOutSend(&m_nsif[systemId]->base, &msg->base, SIMBRICKS_PROTO_MSG_TYPE_SYNC);
  }
  for(int i=0; i<connsRev[delay].size(); i++){
    int systemId = connsRev[delay][i];
    while (Poll (systemId));
  }
  Simulator::Schedule (PicoSeconds (delay), &SimbricksMpiInterface::SendSyncEvent, delay);
}

uint8_t SimbricksMpiInterface::Poll (int systemId)
{
  volatile union SimbricksProtoNetMsg *msg;
  uint8_t ty;

  msg = SimbricksNetIfInPoll (m_nsif[systemId], Simulator::GetMaximumSimulationTime ().ToInteger (Time::PS));
  m_nextTime[systemId] = SimbricksNetIfInTimestamp (m_nsif[systemId]);
  
  if (!msg)
    return -1;

  ty = SimbricksNetIfInType(m_nsif[systemId], msg);
  switch (ty) {
    case SIMBRICKS_PROTO_NET_MSG_PACKET:
      ReceivedPacket ((const void *) msg->packet.data, msg->packet.len, msg->base.header.timestamp);
      break;

    case SIMBRICKS_PROTO_MSG_TYPE_SYNC:
      break;

    default:
      NS_ABORT_MSG ("SimbricksMpiInterface::Poll: unsupported message type " << ty);
  }

  SimbricksNetIfInDone (m_nsif[systemId], msg);
  return ty;
}


void SimbricksMpiInterface::InitMap (){
  NodeContainer c = NodeContainer::GetGlobal ();
  int systemId = MpiInterface::GetSystemId();
  for (NodeContainer::Iterator iter = c.Begin (); iter != c.End (); ++iter)
    {
      if ((*iter)->GetSystemId () != systemId)
        {
          continue;
        }

      for (uint32_t i = 0; i < (*iter)->GetNDevices (); ++i)
        {
          Ptr<NetDevice> localNetDevice = (*iter)->GetDevice (i);
          // only works for p2p links currently
          if (!localNetDevice->IsPointToPoint ())
            {
              continue;
            }
          Ptr<Channel> channel = localNetDevice->GetChannel ();
          if (channel == 0)
            {
              continue;
            }

          // grab the adjacent node
          Ptr<Node> remoteNode;
          if (channel->GetDevice (0) == localNetDevice)
            {
              remoteNode = (channel->GetDevice (1))->GetNode ();
            }
          else
            {
              remoteNode = (channel->GetDevice (0))->GetNode ();
            }

          // if it's not remote, don't consider it
          if (remoteNode->GetSystemId () == systemId)
            {
              continue;
            }

          TimeValue delay;
          channel->GetAttribute ("Delay", delay);
          uint64_t delayps = delay.Get().ToInteger (Time::PS);
          if(conns.find(systemId)!=conns.end() && conns[systemId].find(remoteNode->GetSystemId())!=conns[systemId].end()){
            conns[systemId][remoteNode->GetSystemId ()] = std::min(delayps, conns[systemId][remoteNode->GetSystemId ()]);
            conns[remoteNode->GetSystemId ()][systemId] = std::min(delayps, conns[systemId][remoteNode->GetSystemId ()]);
          }
          else{
            conns[systemId][remoteNode->GetSystemId ()] = delayps;
            conns[remoteNode->GetSystemId ()][systemId] = delayps;
          }

          delayps = conns[systemId][remoteNode->GetSystemId ()];

          if(connsRev.find(delayps)==connsRev.end()){
            connsRev[delayps] = std::vector<uint32_t>(0);
            Simulator::ScheduleNow (&SimbricksMpiInterface::SendSyncEvent, delayps);
          }
          bool flag = true;
          for(int i=0; i<connsRev[delayps].size(); i++){
            if(connsRev[delayps][i]==remoteNode->GetSystemId ())
              flag = false;            
          }
          if(flag)
            connsRev[delayps].push_back(remoteNode->GetSystemId ());
        }
    }
}

void SimbricksMpiInterface::SetupInterconnections (){

  int systemId = MpiInterface::GetSystemId();
  int num_conns = conns[systemId].size();
  ests = (struct SimBricksBaseIfEstablishData*)malloc(sizeof(struct SimBricksBaseIfEstablishData)*num_conns);

  for(auto i = conns[systemId].begin(); i != conns[systemId].end(); i++){
    m_bifparam[i->first] = new SimbricksBaseIfParams();
    SimbricksNetIfDefaultParams(m_bifparam[i->first]);

    m_bifparam[i->first]->sync_mode = (enum SimbricksBaseIfSyncMode)1;
    m_bifparam[i->first]->sync_interval = i->second;
    m_pollDelay[i->first] = Time(PicoSeconds (m_bifparam[i->first]->sync_interval));
    m_bifparam[i->first]->link_latency = i->second;

    int a,b;
    if(systemId>i->first){
      a=i->first;
      b=systemId;
    }
    else{
      a=systemId;
      b=i->first;
    }
    
    std::string shm_path = m_dir+"sim_shm"+std::to_string(a)+"_"+std::to_string(b), sock_path=m_dir+"sim_socket"+std::to_string(a)+"_"+std::to_string(b);
    m_bifparam[i->first]->sock_path = sock_path.c_str();

    int ret;
    int sync = m_bifparam[i->first]->sync_mode;
    m_nsif[i->first] = new SimbricksNetIf();
    if(a==systemId) {while(access(sock_path.c_str(), R_OK));}
    else {
        m_pool[i->first] = new SimbricksBaseIfSHMPool();
        struct SimbricksBaseIf *netif = &m_nsif[i->first]->base;

        // first allocate pool
        size_t shm_size = 3200*8192;
        if (SimbricksBaseIfSHMPoolCreate(m_pool[i->first], shm_path.c_str(), shm_size)) {
        perror("SimbricksNicIfInit: SimbricksBaseIfSHMPoolCreate failed");
        exit(0);
        return;
        }
        if (SimbricksBaseIfInit(netif, m_bifparam[i->first])) {
        perror("SimbricksNicIfInit: SimbricksBaseIfInit net failed");
        exit(0);
        return;
        }
        if (SimbricksBaseIfListen(netif, m_pool[i->first])) {
        perror("SimbricksNicIfInit: SimbricksBaseIfListen net failed");
        exit(0);
        return;
        }
        m_net_intro[i->first] = new SimbricksProtoNetIntro();
        memset(m_net_intro[i->first], 0, sizeof(*m_net_intro[i->first]));
        ests[n_bifs].base_if = netif;
        ests[n_bifs].tx_intro = m_net_intro[i->first];
        ests[n_bifs].tx_intro_len = sizeof(*m_net_intro[i->first]);
        ests[n_bifs].rx_intro = m_net_intro[i->first];
        ests[n_bifs].rx_intro_len = sizeof(*m_net_intro[i->first]);
        
        ++n_bifs;
        continue;
    }
    if(SimbricksNetIfInit(m_nsif[i->first], m_bifparam[i->first], m_bifparam[i->first]->sock_path, &sync)) exit(0);
    // m_pollEvent[i->first] = Simulator::ScheduleNow (&SimbricksMpiInterface::PollEvent, this, i->first);
  }
  if(n_bifs) SimBricksBaseIfEstablish(ests, n_bifs);
  delete[] ests;
}


void
SimbricksMpiInterface::Disable ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_enabled = false;
  m_initialized = false;
}


} // namespace ns3