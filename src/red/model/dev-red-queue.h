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

#include "ns3/queue.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

/**
 * \ingroup queue
 *
 * \brief red queue for netdev
 */
template <typename Item>
class DevRedQueue : public Queue<Item>
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
  DevRedQueue ();

  virtual ~DevRedQueue ();

  virtual bool Enqueue (Ptr<Item> item);
  virtual Ptr<Item> Dequeue (void);
  virtual Ptr<Item> Remove (void);
  virtual Ptr<const Item> Peek (void) const;

private:
  using Queue<Item>::GetContainer;
  using Queue<Item>::DoEnqueue;
  using Queue<Item>::DoDequeue;
  using Queue<Item>::DoRemove;
  using Queue<Item>::DoPeek;

  double m_minTh;

  NS_LOG_TEMPLATE_DECLARE;     //!< redefinition of the log component
};


/**
 * Implementation of the templates declared above.
 */

template <typename Item>
TypeId
DevRedQueue<Item>::GetTypeId (void)
{
  static TypeId tid =
      TypeId (GetTemplateClassName<DevRedQueue<Item>> ())
          .SetParent<Queue<Item>> ()
          .SetGroupName ("Network")
          .template AddConstructor<DevRedQueue<Item>> ()
          .AddAttribute ("MaxSize", "The max queue size", QueueSizeValue (QueueSize ("100p")),
                         MakeQueueSizeAccessor (&QueueBase::SetMaxSize, &QueueBase::GetMaxSize),
                         MakeQueueSizeChecker ())
          .AddAttribute ("MinTh", "Minimum length threshold in Bytes",
                         DoubleValue (200000), MakeDoubleAccessor (&DevRedQueue::m_minTh),
                         MakeDoubleChecker<double> ())
                         ;

  return tid;
}

template <typename Item>
DevRedQueue<Item>::DevRedQueue () :
  Queue<Item> (),
  NS_LOG_TEMPLATE_DEFINE ("DevRedQueue")
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
DevRedQueue<Item>::~DevRedQueue ()
{
  NS_LOG_FUNCTION (this);
}

template <typename Item>
bool
DevRedQueue<Item>::Enqueue (Ptr<Item> item)
{
  NS_LOG_FUNCTION (this << item);
  //NS_LOG_LOGIC ("DevRedQueue Enqueue  " << item);

  Ipv4Header header;
  Ipv4Address destination;
  Ipv4Address source, masked;
  Ipv4Address subnet("192.168.0.0");
  Ipv4Header::EcnType ecn;
  Ipv4Mask mask("255.255.0.0");

  //make a copy of the packet
  Ptr<Packet> q = item->Copy();

  //get the packet header
  item->RemoveHeader (header);

  destination = header.GetDestination ();
  source = header.GetSource ();
  masked = source.CombineMask(mask);

  ecn = header.GetEcn();
  if (masked == subnet){
    NS_LOG_LOGIC("Redqueue size: " << QueueBase::GetCurrentSize() << " nPacket: " << QueueBase::GetNPackets()  << " dst: " << destination << " source: " << source);
    if ((QueueBase::GetNBytes() >= m_minTh) && (ecn != Ipv4Header::ECN_NotECT)){
      NS_LOG_LOGIC("set ECN");
      header.SetEcn (Ipv4Header::ECN_CE);
    }
    
    header.EnableChecksum();
    item->AddHeader (header);

    //NS_LOG_LOGIC ("dst: " << destination << " source: " << source << " ECN: " << header.GetEcn());
    //packet is IPv4 packet, send modified
    return DoEnqueue (GetContainer ().end (), item);
  
  }
  
  //NS_LOG_LOGIC ("dest: " << destination << "  source: " << source << " ECN: " << ecn);
  NS_LOG_INFO("ARP");
  // the packet is ARP, send original packet 
  return DoEnqueue (GetContainer ().end (), q);
}

template <typename Item>
Ptr<Item>
DevRedQueue<Item>::Dequeue (void)
{
  NS_LOG_FUNCTION (this);
  

  Ptr<Item> item = DoDequeue (GetContainer ().begin ());
  

  //NS_LOG_LOGIC ("Popped" << item << "  dest: " << destination << "  source: " << source << " ECN: " << ecn);
  NS_LOG_LOGIC ("Popped" << item);

  return item;
}

template <typename Item>
Ptr<Item>
DevRedQueue<Item>::Remove (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Item> item = DoRemove (GetContainer ().begin ());

  NS_LOG_LOGIC ("Removed " << item);

  return item;
}

template <typename Item>
Ptr<const Item>
DevRedQueue<Item>::Peek (void) const
{
  NS_LOG_FUNCTION (this);

  return DoPeek (GetContainer ().begin ());
}

// The following explicit template instantiation declarations prevent all the
// translation units including this header file to implicitly instantiate the
// DropTailQueue<Packet> class and the DropTailQueue<QueueDiscItem> class. The
// unique instances of these classes are explicitly created through the macros
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,Packet) and
// NS_OBJECT_TEMPLATE_CLASS_DEFINE (DropTailQueue,QueueDiscItem), which are included
// in drop-tail-queue.cc
extern template class DevRedQueue<Packet>;
//extern template class DevRedQueue<QueueDiscItem>;

} // namespace ns3

#endif /* DEVRED_H */
