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

#include "ns3/bridge-helper.h"

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
    NS_ABORT_MSG_UNLESS(type_opt, "Topology node has no type");
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
    config.Set(MakeCallback(&BridgeHelper::SetDeviceAttribute, &bridgeHelper));

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
    NS_ABORT_MSG_UNLESS(type_opt, "Topology channel has no type");
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

static void
SetQueue(SimpleNetDeviceHelper* helper,
         const std::string& type,
         const std::string& key,
         const AttributeValue& val)
{
    helper->SetQueue(type, key, val);
}

E2ESimpleChannel::E2ESimpleChannel(const E2EConfig& config)
    : E2ETopologyChannel(config)
{
    auto categories {config.ParseCategories()};
    if (auto it {categories.find("Device")}; it != categories.end())
    {
        config.Set(MakeCallback(&SimpleNetDeviceHelper::SetDeviceAttribute, &m_channelHelper),
                   it->second);
    }

    std::string_view channelType;
    if (auto opt {config.Find("ChannelType")}; opt)
    {
        channelType = opt->value;
        opt->processed = true;
    }
    else
    {
        channelType = "ns3::SimpleChannel";
    }
    m_channelHelper.SetChannel(std::string(channelType));
    if (auto it {categories.find("Channel")}; it != categories.end())
    {
        config.Set(MakeCallback(&SimpleNetDeviceHelper::SetChannelAttribute, &m_channelHelper),
                   it->second);
    }

    std::string queueType;
    if (auto opt {config.Find("QueueType")}; opt)
    {
        queueType = opt->value;
        opt->processed = true;
    }
    else
    {
        queueType = "ns3::DropTailQueue";
    }
    if (auto it {categories.find("Queue")}; it != categories.end())
    {
        config.Set(MakeBoundCallback(&SetQueue, &m_channelHelper, queueType), it->second);
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