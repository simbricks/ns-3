/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
 *
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
 */

#ifndef DEVRED_H
#define DEVRED_H

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/in.h>

#include "ns3/queue.h"
#include "ipv4-header.h"
#include "ns3/header.h"
#include "ppp-header.h"


namespace ns3 {

/**
 * \ingroup queue
 *
 * \brief red queue for netdev
 */
template <typename Item>
class DevRedQueuePPP : public Queue<Item>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief DropTailQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  DevRedQueuePPP ();

  virtual ~DevRedQueuePPP ();

  virtual bool Enqueue (Ptr<Item> item);
  virtual Ptr<Item> Dequeue (void);
  virtual Ptr<Item> Remove (void);
  virtual Ptr<const Item> Peek (void) const;

private:
  using Queue<Item>::begin;
  using Queue<Item>::end;
  using Queue<Item>::DoEnqueue;
  using Queue<Item>::DoDequeue;
  using Queue<Item>::DoRemove;
  using Queue<Item>::DoPeek;

  double m_minTh;
  double m_meanPktSize;

  NS_LOG_TEMPLATE_DECLARE;     //!< redefinition of the log component
};


/**
 * Implementation of the templates declared above.
 */

template <typename Item>
TypeId
DevRedQueuePPP<Item>::GetTypeId (void)
{
  static TypeId tid =
      TypeId (("ns3::DevRedQueuePPP<" + GetTypeParamName<DevRedQueuePPP<Item>> () + ">").c_str ())
          .SetParent<Queue<Item>> ()
          .SetGroupName ("Network")
          .template AddConstructor<DevRedQueuePPP<Item>> ()
          .AddAttribute ("MaxSize", "The max queue size", QueueSizeValue (QueueSize ("100p")),
                         MakeQueueSizeAccessor (&QueueBase::SetMaxSize, &QueueBase::GetMaxSize),
                         MakeQueueSizeChecker ())
          .AddAttribute ("MinTh", "Minimum length threshold in Bytes",
                         DoubleValue (200000), MakeDoubleAccessor (&DevRedQueuePPP::m_minTh),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("MeanPktSize", "Mean Pkt Size",
                        DoubleValue (1500), MakeDoubleAccessor (&DevRedQueuePPP::m_meanPktSize),
                        MakeDoubleChecker<double> ())
                      ;

  return tid;
}

template <typename Item>
DevRedQueuePPP<Item>::DevRedQueuePPP () :
  Queue<Item> (),
  NS_LOG_TEMPLATE_DEFINE ("DevRedQueuePPP")
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
DevRedQueuePPP<Item>::~DevRedQueuePPP ()
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
bool
DevRedQueuePPP<Item>::Enqueue (Ptr<Item> item)
{
  NS_LOG_FUNCTION (this << item);
  //NS_LOG_LOGIC ("DevRedQueuePPP Enqueue  " << item);

  Ipv4Header header;
  PppHeader pheader;
  Ipv4Address source, destination;
  Ipv4Header::EcnType ecn;

  //make a copy of the packet
  Ptr<Packet> q = item->Copy();
  

  /*size_t buf_size = q->GetSize();
  uint8_t *buffer = new uint8_t[buf_size];
  q->CopyData(buffer, buf_size);
  uint8_t *pktptr = buffer;
  pktptr += 2;
  Ipv4Header header_;
  struct iphdr *iph = (struct iphdr*) pktptr;
  uint32_t ip_source = iph->saddr;
  NS_LOG_DEBUG("IP source: " << ip_source);
  Ptr<Packet> pkt_tosend = Create<Packet> (pktptr, buf_size - 2);
  pkt_tosend->RemoveHeader(header_);
  NS_LOG_DEBUG("source: " << header_.GetSource());  
  */


  //get the packet header
  item->RemoveHeader (pheader);
  uint16_t proto = pheader.GetProtocol();
  NS_LOG_DEBUG("PPP header protocol: " << proto);
  item->RemoveHeader (header);

  destination = header.GetDestination ();
  source = header.GetSource ();  

  ecn = header.GetEcn();

  if (proto == 0x0021){
    NS_LOG_LOGIC("Redqueue size: " << QueueBase::GetCurrentSize() << " nPacket: " << QueueBase::GetNPackets()  << " dst: " << destination << " source: " << source);
    int pkt_num = m_minTh / m_meanPktSize;


    double k_val = (m_meanPktSize + 2) * pkt_num;
    NS_LOG_DEBUG("k_val: " << k_val << " pkt_num: " << pkt_num);
    if ((QueueBase::GetNBytes() >= k_val) && (ecn != Ipv4Header::ECN_NotECT)){
      NS_LOG_LOGIC("set ECN");
      header.SetEcn (Ipv4Header::ECN_CE);
    }
    
    header.EnableChecksum();
    item->AddHeader (header);
    item->AddHeader(pheader);

    //packet is IPv4 packet, send modified
    return DoEnqueue (end (), item);
  
  }
  
  return DoEnqueue (end (), q);
}

template <typename Item>
Ptr<Item>
DevRedQueuePPP<Item>::Dequeue (void)
{
  NS_LOG_FUNCTION (this);
  

  Ptr<Item> item = DoDequeue (begin ());
  

  //NS_LOG_LOGIC ("Popped" << item << "  dest: " << destination << "  source: " << source << " ECN: " << ecn);
  NS_LOG_LOGIC ("Popped" << item);

  return item;
}

template <typename Item>
Ptr<Item>
DevRedQueuePPP<Item>::Remove (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Item> item = DoRemove (begin ());

  NS_LOG_LOGIC ("Removed " << item);

  return item;
}

template <typename Item>
Ptr<const Item>
DevRedQueuePPP<Item>::Peek (void) const
{
  NS_LOG_FUNCTION (this);

  return DoPeek (begin ());
}

// The following explicit template instantiation declarations prevent all the
// translation units including this header file to implicitly instantiate the
// DropTailQueue<Packet> class and the DropTailQueue<QueueDiscItem> class. The
// unique instances of these classes are explicitly created through the macros
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,Packet) and
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,QueueDiscItem), which are included
// in drop-tail-queue.cc
extern template class DevRedQueuePPP<Packet>;
//extern template class DevRedQueuePPP<QueueDiscItem>;

} // namespace ns3

#endif /* DEVRED_H */
