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

#include "e2e-network.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2ENetwork");

E2ENetwork::E2ENetwork(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().empty(), "Network has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 2,
        "Network '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().empty(), "Network '" << GetId() << "' has no type");
}

Ptr<E2ENetwork>
E2ENetwork::CreateNetwork(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt.has_value(), "Host has no type");
    std::string_view type {*type_opt};

    if (type == "NetIf")
    {
        return Create<E2ENetworkNetIf>(config);
    }
    else if (type == "NicIf")
    {
        return Create<E2ENetworkNicIf>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown network type '" << type << "'");
    }
}

Ptr<NetDevice>
E2ENetwork::GetNetDevice()
{
    return m_netDevice;
}

E2ENetworkNetIf::E2ENetworkNetIf(const E2EConfig& config) : E2ENetwork(config)
{
    Ptr<simbricks::SimbricksNetDevice> netDevice =
      CreateObject<simbricks::SimbricksNetDevice>();
    if (not config.SetAttrIfContained<StringValue, std::string>(netDevice,
        "UnixSocket", "UnixSocket"))
    {
        NS_LOG_WARN("No Unix socket path for Simbricks host '" << GetId() << "' given.");
    }
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "SyncDelay", "SyncDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "PollDelay", "PollDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "EthLatency", "EthLatency");
    config.SetAttrIfContained<IntegerValue, int>(netDevice, "Sync", "Sync");
    config.SetAttrIfContained<StringValue, std::string>(netDevice, "Listen", "Listen");
    config.SetAttrIfContained<StringValue, std::string>(netDevice, "ShmPath", "ShmPath");

    netDevice->Start();

    m_netDevice = netDevice;
}

E2ENetworkNicIf::E2ENetworkNicIf(const E2EConfig& config) : E2ENetwork(config)
{
    Ptr<simbricks::SimbricksNetDevice> netDevice =
      CreateObject<simbricks::SimbricksNetDevice>();
    if (not config.SetAttrIfContained<StringValue, std::string>(netDevice,
        "UnixSocket", "UnixSocket"))
    {
        NS_LOG_WARN("No Unix socket path for Simbricks host '" << GetId() << "' given.");
    }
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "SyncDelay", "SyncDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "PollDelay", "PollDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "EthLatency", "EthLatency");
    config.SetAttrIfContained<IntegerValue, int>(netDevice, "Sync", "Sync");
    config.SetAttrIfContained<StringValue, std::string>(netDevice, "ShmPath", "ShmPath");

    netDevice->SetAttribute("Listen", BooleanValue(true));

    netDevice->Start();

    m_netDevice = netDevice;

}

E2ENetworkTrunk::E2ENetworkTrunk(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().empty(), "Trunk has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 1,
        "Trunk '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    
    m_trunk = CreateObject<simbricks::SimbricksTrunk>();

    if (not config.SetAttrIfContained<StringValue, std::string>(m_trunk,
        "UnixSocket", "UnixSocket"))
    {
        NS_LOG_WARN("No Unix socket path for Simbricks trunk '" << GetId() << "' given.");
    }
    config.SetAttrIfContained<TimeValue, Time>(m_trunk, "SyncDelay", "SyncDelay");
    config.SetAttrIfContained<TimeValue, Time>(m_trunk, "PollDelay", "PollDelay");
    config.SetAttrIfContained<TimeValue, Time>(m_trunk, "EthLatency", "EthLatency");
    config.SetAttrIfContained<IntegerValue, int>(m_trunk, "Sync", "Sync");
    config.SetAttrIfContained<StringValue, std::string>(m_trunk, "Listen", "Listen");
    config.SetAttrIfContained<StringValue, std::string>(m_trunk, "ShmPath", "ShmPath");

    m_trunk->Start();
}

Ptr<NetDevice>
E2ENetworkTrunk::AddDevice(int id)
{
    NS_ABORT_MSG_IF(id <= m_current_id,
        "Wrong ordering of trunk devices, current id: " << m_current_id << ", given id: " << id
        << ", for trunk: " << GetId());
    m_current_id = id;
    return m_trunk->AddNetDev();
}

E2ENetworkTrunkDevice::E2ENetworkTrunkDevice(const E2EConfig& config,
                                             Ptr<E2EComponent> root,
                                             int64_t orderId)
    : E2ENetwork(config)
{
    auto trunkId = config.Find("Trunk");
    NS_ABORT_MSG_UNLESS(trunkId,
        "Trunk device '" << GetId() << "' does not contain a trunk to use");

    auto trunk = root->GetE2EComponent<E2ENetworkTrunk>(*trunkId);
    NS_ABORT_MSG_UNLESS(trunk, "Trunk '" << *trunkId << "' not found");

    Ptr<NetDevice> netDevice = (*trunk)->AddDevice(orderId);
    NS_ABORT_MSG_UNLESS(netDevice, "Failed to add device to trunk '" << *trunkId << "'");

    m_netDevice = netDevice;
}

} // namespace ns3
