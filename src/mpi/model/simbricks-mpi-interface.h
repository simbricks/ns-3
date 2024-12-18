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

#ifndef SIMBRICKS_MPI_INTERFACE_H
#define SIMBRICKS_MPI_INTERFACE_H

#include <stdint.h>
#include <list>

#include "ns3/nstime.h"
#include "ns3/buffer.h"
#include "ns3/mpi-receiver.h"
#include "ns3/simulator-impl.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/node-container.h"

#include <ns3/channel.h>
#include <chrono>
#include <list>
#include <string>
#include <map>
#include <vector>
#include <simbricks/base/cxxatomicfix.h>
#include <unistd.h>
extern "C" {
#include <simbricks/network/if.h>
#include <simbricks/network/proto.h>
#include <simbricks/nicif/nicif.h>
}

#include "parallel-communication-interface.h"

namespace ns3 {



/**
 * \ingroup mpi
 *
 * \brief Interface between ns-3 and MPI
 *
 * Implements the interface used by the singleton parallel controller
 * to interface between NS3 and the communications layer being
 * used for inter-task packet transfers.
 */
class SimbricksMpiInterface : public ParallelCommunicationInterface, Object
{
public:
  static TypeId GetTypeId (void);

  /**
   * Delete all buffers
   */
  virtual void Destroy ();
  /**
   * \return MPI rank
   */
  virtual uint32_t GetSystemId ();
  /**
   * \return MPI size (number of systems)
   */
  virtual uint32_t GetSize ();
  /**
   * \return true if using MPI
   */
  virtual bool IsEnabled ();
  /**
   * \param pargc number of command line arguments
   * \param pargv command line arguments
   *
   * Sets up MPI interface
   */
  virtual void Enable (int* pargc, char*** pargv);
  /**
   * Terminates the MPI environment by calling MPI_Finalize
   * This function must be called after Destroy ()
   * It also resets m_initialized, m_enabled
   */
  virtual void Disable ();
  /**
   * \param p packet to send
   * \param rxTime received time at destination node
   * \param node destination node
   * \param dev destination device
   *
   * Serialize and send a packet to the specified node and net device
   */
  virtual void SendPacket (Ptr<Packet> p, const Time &rxTime, uint32_t node, uint32_t dev);

  static uint32_t m_sid;
  static bool     m_initialized;
  static bool     m_enabled;
  static uint32_t m_size;
  static std::map<uint32_t, SimbricksNetIf*> m_nsif;
  static std::map<uint32_t, bool> m_isConnected;
  static std::map<uint32_t, uint64_t> m_nextTime;
  static std::map<uint32_t, EventId> m_syncTxEvent;
  static std::map<uint32_t, EventId> m_pollEvent;
  static bool m_syncMode;
  static std::map<uint32_t, std::map<uint32_t,uint64_t>> conns;
  static std::map<uint64_t, std::vector<uint32_t>> connsRev;
  static std::map<uint32_t, SimbricksBaseIfParams*> m_bifparam;
  static std::map<uint32_t, Time> m_pollDelay;
  static std::string m_dir;
  static struct SimBricksBaseIfEstablishData *ests;
  static unsigned n_bifs;
  static std::map<uint32_t, SimbricksProtoNetIntro*> m_net_intro;
  static std::map<uint32_t, SimbricksBaseIfSHMPool*> m_pool;

  static volatile union SimbricksProtoNetMsg *AllocTx (int systemId);
  static void ReceivedPacket (const void *buf, size_t len, uint64_t time);
  static uint8_t Poll (int systemId);
  static void InitMap (void);
  static void SetupInterconnections (void);
  static void SendSyncEvent (uint64_t delay);
};

} // namespace ns3

#endif /* NS3_GRANTED_TIME_WINDOW_MPI_INTERFACE_H */
