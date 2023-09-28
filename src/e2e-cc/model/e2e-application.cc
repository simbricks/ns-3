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

#include "e2e-application.h"
#include "e2e-component.h"
#include "e2e-config.h"

#include "ns3/applications-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2EApplication");

E2EApplication::E2EApplication(const E2EConfig& config) : E2EComponent(config)
{
    NS_ABORT_MSG_IF(GetId().size() == 0, "Application has no id");
    NS_ABORT_MSG_IF(GetIdPath().size() != 3,
        "Application '" << GetId() << "' has invalid path length of " << GetIdPath().size());
    NS_ABORT_MSG_IF(GetType().size() == 0, "Application '" << GetId() << "' has no type");

    config.SetFactoryIfContained<TimeValue, Time>(m_factory, "StartTime", "StartTime");
    config.SetFactoryIfContained<TimeValue, Time>(m_factory, "StopTime", "StopTime");
}

Ptr<E2EApplication>
E2EApplication::CreateApplication(const E2EConfig& config)
{
    auto type_opt {config.Find("Type")};
    NS_ABORT_MSG_UNLESS(type_opt.has_value(), "Application has no type");
    std::string_view type {*type_opt};

    if (type == "PacketSink")
    {
        return Create<E2EPacketSink>(config);
    }
    else if (type == "BulkSender")
    {
        return Create<E2EBulkSender>(config);
    }
    else
    {
        NS_ABORT_MSG("Unkown application type '" << type << "'");
    }
}

Ptr<Application> E2EApplication::GetApplication()
{
    return m_application;
}

E2EPacketSink::E2EPacketSink(const E2EConfig& config) : E2EApplication(config)
{
    m_factory.SetTypeId("ns3::PacketSink");
    if (not config.SetFactoryIfContained<StringValue, std::string>(m_factory,
        "Protocol", "Protocol"))
    {
        NS_ABORT_MSG("Packet sink '" << GetId() << "' requires a protocol");
    }
    if (not config.SetFactoryIfContained<AddressValue, InetSocketAddress>(m_factory,
        "Local", "Local"))
    {
        NS_ABORT_MSG("Packet sink '" << GetId() << "' requires a local address");
    }
    m_application = m_factory.Create<Application>();
}

E2EBulkSender::E2EBulkSender(const E2EConfig& config) : E2EApplication(config)
{
    m_factory.SetTypeId("ns3::BulkSendApplication");
    if (not config.SetFactoryIfContained<StringValue, std::string>(m_factory,
        "Protocol", "Protocol"))
    {
        NS_ABORT_MSG("Bulk send application '" << GetId() << "' requires a protocol");
    }
    if (not config.SetFactoryIfContained<AddressValue, InetSocketAddress>(m_factory,
        "Remote", "Remote"))
    {
        NS_ABORT_MSG("Bulk send application '" << GetId() << "' requires a remote address");
    }
    config.SetFactoryIfContained<UintegerValue, unsigned>(m_factory, "SendSize", "SendSize");
    config.SetFactoryIfContained<UintegerValue, unsigned>(m_factory, "MaxBytes", "MaxBytes");
    m_application = m_factory.Create<Application>();
}

} // namespace ns3