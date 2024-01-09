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

#include "ns3/simbricks-module.h"

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
    Ptr<SimbricksNetDevice> netDevice = CreateObject<SimbricksNetDevice>();
    if (not config.SetAttrIfContained<StringValue, std::string>(netDevice,
        "UnixSocket", "UnixSocket"))
    {
        NS_LOG_WARN("No Unix socket path for Simbricks host '" << GetId() << "' given.");
    }
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "SyncDelay", "SyncDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "PollDelay", "PollDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "EthLatency", "EthLatency");
    config.SetAttrIfContained<IntegerValue, int>(netDevice, "Sync", "Sync");

    netDevice->Start();
    
    m_netDevice = netDevice;
}

E2ENetworkNicIf::E2ENetworkNicIf(const E2EConfig& config) : E2ENetwork(config)
{
    Ptr<SimbricksNetDeviceNicIf> netDevice = CreateObject<SimbricksNetDeviceNicIf>();
    if (not config.SetAttrIfContained<StringValue, std::string>(netDevice,
        "UnixSocket", "UnixSocket"))
    {
        NS_LOG_WARN("No Unix socket path for Simbricks host '" << GetId() << "' given.");
    }
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "SyncDelay", "SyncDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "PollDelay", "PollDelay");
    config.SetAttrIfContained<TimeValue, Time>(netDevice, "EthLatency", "EthLatency");
    config.SetAttrIfContained<IntegerValue, int>(netDevice, "Sync", "Sync");

    netDevice->Start();
    
    m_netDevice = netDevice;
}

} // namespace ns3