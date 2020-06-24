/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#pragma once

#include "ns3/sequencer.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

class Node;

class SequencerHelper
{
public:
  SequencerHelper ();

  NetDeviceContainer Install (Ptr<Node> node,
                              NetDeviceContainer replicas,
                              NetDeviceContainer non_replicas);

private:
  ObjectFactory m_deviceFactory;
};

} // namespace ns3
