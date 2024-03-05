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

#include "e2e-host.h"

#include "ns3/config.h"
#include "ns3/integer.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/queue.h"
#include "ns3/simbricks-netdev.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2EHost");

E2EHost::E2EHost(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().empty(), "Host has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 2,
        "Host '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().empty(), "Host '" << GetId() << "' has no type");
}

Ptr<E2EHost>
E2EHost::CreateHost(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt, "Host has no type");
    std::string_view type {(*type_opt).value};
    (*type_opt).processed = true;

    if (type == "Simbricks")
    {
        return Create<E2ESimbricksHost>(config);
    }
    else if (type == "SimpleNs3")
    {
        return Create<E2ESimpleNs3Host>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown host type '" << type << "'");
    }
}

Ptr<NetDevice>
E2EHost::GetNetDevice()
{
    return m_netDevice;
}

Ptr<Node>
E2EHost::GetNode()
{
    return Ptr<Node>();
}

void
E2EHost::AddApplication(Ptr<E2EApplication> application)
{
    NS_ABORT_MSG("Applications are not supported for host '" << GetId()
        << "' with type '" << GetType() << "'");
}

E2ESimbricksHost::E2ESimbricksHost(const E2EConfig& config) : E2EHost(config)
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
    config.SetAttr(netDevice);

    netDevice->Start();

    m_netDevice = netDevice;
}

E2ESimpleNs3Host::E2ESimpleNs3Host(const E2EConfig& config) : E2EHost(config)
{
    m_node = CreateObject<Node>();
    auto categories {config.ParseCategories()};

    ObjectFactory deviceFactory;
    deviceFactory.SetTypeId("ns3::SimpleNetDevice");
    if (auto it {categories.find("Device")}; it != categories.end())
    {
        config.SetFactory(deviceFactory, it->second);
    }

    ObjectFactory queueFactory;
    if (auto opt {config.Find("QueueType")}; opt)
    {
        std::string queueType {opt->value};
        QueueBase::AppendItemTypeIfNotPresent(queueType, "Packet");
        queueFactory.SetTypeId(queueType);
        opt->processed = true;
    }
    else
    {
        queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    }
    if (auto it {categories.find("Queue")}; it != categories.end())
    {
        config.SetFactory(queueFactory, it->second);
    }

    ObjectFactory channelFactory;
    if (auto opt {config.Find("ChannelType")}; opt)
    {
        channelFactory.SetTypeId(std::string(opt->value));
        opt->processed = true;
    }
    else
    {
        channelFactory.SetTypeId("ns3::SimpleChannel");
    }
    if (auto it {categories.find("Channel")}; it != categories.end())
    {
        config.SetFactory(channelFactory, it->second);
    }

    // Create net devices
    Ptr<SimpleNetDevice> netDevice = deviceFactory.Create<SimpleNetDevice>();
    m_netDevice = netDevice;
    m_outerNetDevice = deviceFactory.Create<SimpleNetDevice>();
    //device->SetAttribute("PointToPointMode", BooleanValue(m_pointToPointMode));
    // Allocate and set mac addresses
    m_netDevice->SetAddress(Mac48Address::Allocate());
    m_outerNetDevice->SetAddress(Mac48Address::Allocate());

    m_node->AddDevice(m_outerNetDevice);

    m_channel = channelFactory.Create<SimpleChannel>();
    netDevice->SetChannel(m_channel);
    m_outerNetDevice->SetChannel(m_channel);
    Ptr<Queue<Packet>> innerQueue = queueFactory.Create<Queue<Packet>>();
    netDevice->SetQueue(innerQueue);
    Ptr<Queue<Packet>> outerQueue = queueFactory.Create<Queue<Packet>>();
    m_outerNetDevice->SetQueue(outerQueue);
    
    if (m_enableFlowControl)
    {
        // Aggregate a NetDeviceQueueInterface object
        Ptr<NetDeviceQueueInterface> innerNdqi = CreateObject<NetDeviceQueueInterface>();
        innerNdqi->GetTxQueue(0)->ConnectQueueTraces(innerQueue);
        m_netDevice->AggregateObject(innerNdqi);

        Ptr<NetDeviceQueueInterface> outerNdqi = CreateObject<NetDeviceQueueInterface>();
        outerNdqi->GetTxQueue(0)->ConnectQueueTraces(outerQueue);
        m_outerNetDevice->AggregateObject(outerNdqi);
    }

    // Set congestion control algorithm
    if (auto algo {config.Find("CongestionControl")}; algo)
    {
        TypeId tid = TypeId::LookupByName(std::string((*algo).value));
        std::stringstream nodeId;
        nodeId << m_node->GetId();
        std::string specificNode = "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";
        Config::Set(specificNode, TypeIdValue(tid));
        (*algo).processed = true;
    }

    SetIpAddress();
}

Ptr<Node>
E2ESimpleNs3Host::GetNode()
{
    return m_node;
}

void
E2ESimpleNs3Host::AddApplication(Ptr<E2EApplication> application)
{
    m_node->AddApplication(application->GetApplication());
    AddE2EComponent(application);
}

void
E2ESimpleNs3Host::SetIpAddress()
{
    std::string_view ipString;
    if (auto ip {m_config.Find("Ip")}; ip)
    {
        ipString = (*ip).value;
        (*ip).processed = true;
    }
    else
    {
        NS_LOG_WARN("No IP configuration found for node '" << GetId() << "'");
        return;
    }

    std::string_view ip {ipString};
    std::string_view netmask {ipString};
    auto pos {ip.find('/')};
    NS_ABORT_MSG_IF(pos == std::string_view::npos,
        "IP '" << ipString << "' for node '" << GetId() << "' is invalid");
    ip.remove_suffix(ip.size() - pos);
    netmask.remove_prefix(pos);

    InternetStackHelper stack;
    stack.Install(m_node);

    Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
    NS_ASSERT_MSG(ipv4,
                    "NetDevice is associated"
                    " with a node without IPv4 stack installed -> fail "
                    "(maybe need to use InternetStackHelper?)");

    int32_t interface = ipv4->GetInterfaceForDevice(m_outerNetDevice);
    if (interface == -1)
    {
        interface = ipv4->AddInterface(m_outerNetDevice);
    }
    NS_ASSERT_MSG(interface >= 0, "Interface index not found");

    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress(Ipv4Address(std::string(ip).c_str()),
        Ipv4Mask(std::string(netmask).c_str()));
    ipv4->AddAddress(interface, ipv4Addr);
    ipv4->SetMetric(interface, 1);
    ipv4->SetUp(interface);
}

} // namespace ns3
