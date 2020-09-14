/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "sequencer-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/names.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SequencerHelper");


SequencerHelper::SequencerHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::SequencerNetDevice");
}

NetDeviceContainer
SequencerHelper::Install (Ptr<Node> node,
                          NetDeviceContainer servers,
                          NetDeviceContainer clients,
                          NetDeviceContainer endhostSequencers)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_LOGIC ("**** Install sequencer device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<SequencerNetDevice> dev = m_deviceFactory.Create<SequencerNetDevice> ();
  devs.Add (dev);
  node->AddDevice (dev);

  for (NetDeviceContainer::Iterator i = servers.Begin (); i != servers.End (); ++i) {
      NS_LOG_LOGIC ("**** Add SwitchPort "<< *i);
      dev->AddSwitchPort (*i, true, false);
  }
  for (NetDeviceContainer::Iterator i = clients.Begin ();
       i != clients.End (); ++i) {
      NS_LOG_LOGIC ("**** Add SwitchPort "<< *i);
      dev->AddSwitchPort (*i, false, false);
  }
  for (NetDeviceContainer::Iterator i = endhostSequencers.Begin ();
      i != endhostSequencers.End (); ++i) {
      NS_LOG_LOGIC ("**** Add SwitchPort "<< *i);
      dev->AddSwitchPort (*i, false, true);
  }

  return devs;
}

} // namespace ns3
