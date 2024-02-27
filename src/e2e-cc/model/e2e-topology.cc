/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2023 Max Planck Institute for Software Systems
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

#include "e2e-topology.h"

#include "ns3/core-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2ETopology");

E2ETopologyNode::E2ETopologyNode(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().empty(), "Topology node has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 1,
        "Topology node '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().empty(), "Topology node '" << GetId() << "' has no type");

    m_node = CreateObject<Node>();
}

Ptr<E2ETopologyNode>
E2ETopologyNode::CreateTopologyNode(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt.has_value(), "Topology node has no type");
    std::string_view type {(*type_opt).value};
    (*type_opt).processed = true;

    if (type == "Switch")
    {
        return Create<E2ESwitchNode>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown topology node type '" << type << "'");
    }
}

Ptr<Node>
E2ETopologyNode::GetNode()
{
    return m_node;
}

E2ESwitchNode::E2ESwitchNode(const E2EConfig& config) : E2ETopologyNode(config)
{
    BridgeHelper bridgeHelper;

    if (auto mtu {config.Find("Mtu")}; mtu)
    {
        bridgeHelper.SetDeviceAttribute("Mtu", StringValue(std::string((*mtu).value)));
        (*mtu).processed = true;
    }

    NetDeviceContainer switchContainer = bridgeHelper.Install(m_node, NetDeviceContainer());
    m_switch = StaticCast<BridgeNetDevice>(switchContainer.Get(0));
}

void
E2ESwitchNode::AddHost(Ptr<E2EHost> host)
{
    m_node->AddDevice(host->GetNetDevice());
    m_switch->AddBridgePort(host->GetNetDevice());
    m_hostDevices.Add(host->GetNetDevice());

    AddE2EComponent(host);
}

void
E2ESwitchNode::AddNetwork(Ptr<E2ENetwork> network)
{
    m_node->AddDevice(network->GetNetDevice());
    m_switch->AddBridgePort(network->GetNetDevice());
    m_networkDevices.Add(network->GetNetDevice());

    AddE2EComponent(network);
}

void
E2ESwitchNode::AddChannel(Ptr<NetDevice> channelDevice)
{
    m_switch->AddBridgePort(channelDevice);
    m_linkDevices.Add(channelDevice);
}

E2ETopologyChannel::E2ETopologyChannel(const E2EConfig& config)
    : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().empty(), "Topology channel has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 1,
        "Topology channel '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().empty(), "Topology channel '" << GetId() << "' has no type");
}

Ptr<E2ETopologyChannel>
E2ETopologyChannel::CreateTopologyChannel(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt.has_value(), "Topology channel has no type");
    std::string_view type {(*type_opt).value};
    (*type_opt).processed = true;

    if (type == "Simple")
    {
        return Create<E2ESimpleChannel>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown topology channel type '" << type << "'");
    }
}

E2ESimpleChannel::E2ESimpleChannel(const E2EConfig& config)
    : E2ETopologyChannel(config)
{
    if (auto opt {config.Find("DataRate")}; opt)
    {
        m_channelHelper.SetDeviceAttribute("DataRate",
            DataRateValue(DataRate(std::string((*opt).value))));
        (*opt).processed = true;
    }
    if (auto opt {config.Find("QueueSize")}; opt)
    {
        m_channelHelper.SetQueue("ns3::DropTailQueue", "MaxSize",
            QueueSizeValue(QueueSize(std::string((*opt).value))));
        (*opt).processed = true;
    }
    if (auto opt {config.Find("Delay")}; opt)
    {
        m_channelHelper.SetChannelAttribute("Delay", TimeValue(Time(std::string((*opt).value))));
        (*opt).processed = true;
    }
}

void
E2ESimpleChannel::Connect(Ptr<E2EComponent> root)
{
    auto leftNodeId {m_config.Find("LeftNode")};
    NS_ABORT_MSG_UNLESS(leftNodeId, "No left node for channel " << GetId() << " given");
    (*leftNodeId).processed = true;
    auto rightNodeId {m_config.Find("RightNode")};
    NS_ABORT_MSG_UNLESS(rightNodeId, "No right node for channel " << GetId() << " given");
    (*rightNodeId).processed = true;

    auto leftNode {root->GetE2EComponent<E2ETopologyNode>((*leftNodeId).value)};
    NS_ABORT_MSG_UNLESS(leftNode,
        "Left node " << (*leftNodeId).value << " for channel " << GetId() << " not found");
    auto rightNode {root->GetE2EComponent<E2ETopologyNode>((*rightNodeId).value)};
    NS_ABORT_MSG_UNLESS(rightNode,
        "Right node " << (*rightNodeId).value << " for channel " << GetId() << " not found");

    NodeContainer nodes;
    nodes.Add((*leftNode)->GetNode());
    nodes.Add((*rightNode)->GetNode());
    m_devices = m_channelHelper.Install(nodes);
    (*leftNode)->AddChannel(m_devices.Get(0));
    (*rightNode)->AddChannel(m_devices.Get(1));
}

} // namespace ns3