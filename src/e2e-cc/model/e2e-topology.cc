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

#include "ns3/bridge-module.h"
#include "ns3/core-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2ETopology");

E2ETopology::E2ETopology(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().size() == 0, "Topology has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 1,
        "Topology '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().size() == 0, "Topology '" << GetId() << "' has no type");
}

Ptr<E2ETopology>
E2ETopology::CreateTopology(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt.has_value(), "Topology has no type");
    std::string_view type {*type_opt};

    if (type == "Dumbbell")
    {
        return Create<E2EDumbbellTopology>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown topology type '" << type << "'");
    }
}

E2EDumbbellTopology::E2EDumbbellTopology(const E2EConfig &config) : E2ETopology(config)
{
    m_switches.Create(2);

    SimpleNetDeviceHelper bottleneckHelper;
    if (auto opt {config.Find("DataRate")}; opt)
    {
        bottleneckHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate(std::string(*opt))));
    }
    if (auto opt {config.Find("QueueSize")}; opt)
    {
        bottleneckHelper.SetQueue("ns3::DropTailQueue", "MaxSize",
            QueueSizeValue(QueueSize(std::string(*opt))));
    }
    if (auto opt {config.Find("Delay")}; opt)
    {
        bottleneckHelper.SetChannelAttribute("Delay", TimeValue(Time(std::string(*opt))));
    }
    //bottleneckHelper.SetDeviceAttribute ("Mtu", UintegerValue(1500));

    m_bottleneckDevices = bottleneckHelper.Install(m_switches);

    BridgeHelper bridgeHelper;
    m_switchDevices.Add(bridgeHelper.Install(m_switches.Get(0),
        NetDeviceContainer(m_bottleneckDevices.Get(0))).Get(0));
    m_switchDevices.Add(bridgeHelper.Install(m_switches.Get(1),
        NetDeviceContainer(m_bottleneckDevices.Get(1))).Get(0));
}

void E2EDumbbellTopology::AddHost(Ptr<E2EHost> host)
{
    auto position {host->GetConfig().Find("NodePosition")};
    NS_ABORT_MSG_UNLESS(position.has_value(),
        "No position given for host '" << host->GetId() << "'");

    bool left;

    if (*position == "left")
    {
        left = true;
    }
    else if (*position == "right")
    {
        left = false;
    }
    else
    {
        NS_ABORT_MSG("Unkown position '" << *position << "' for host '" << host->GetId() << "'");
    }

    if (left)
    {
        m_switches.Get(0)->AddDevice(host->GetNetDevice());
        StaticCast<BridgeNetDevice>(m_switchDevices.Get(0))->AddBridgePort(host->GetNetDevice());
    }
    else
    {
        m_switches.Get(1)->AddDevice(host->GetNetDevice());
        StaticCast<BridgeNetDevice>(m_switchDevices.Get(1))->AddBridgePort(host->GetNetDevice());
    }

    AddE2EComponent(host);
}

} // namespace ns3