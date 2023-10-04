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

#include "e2e-component.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2EComponent");

E2EComponent::E2EComponent(const E2EConfig &config) : m_config{config}
{
    auto id = config.Find("Id");
    if (id)
    {
        m_id = *id;
        m_idPath = SplitIdPath(*id);
    }

    auto type = config.Find("Type");
    if (type)
    {
        m_type = *type;
    }
}

std::string_view
E2EComponent::GetId() const
{
    return m_id;
}

std::string_view
E2EComponent::GetComponentId() const
{
    if (m_idPath.size() > 0)
    {
        return m_idPath.back();
    }
    return "";
}

std::string_view
E2EComponent::GetType() const
{
    return m_type;
}

const E2EConfig&
E2EComponent::GetConfig() const
{
    return m_config;
}

const std::vector<std::string_view>&
E2EComponent::GetIdPath() const
{
    return m_idPath;
}

void
E2EComponent::AddProbe(const E2EConfig& config)
{
    NS_ABORT_MSG("Adding probes to '" << m_id << "' is not supported");
}

void
E2EComponent::AddE2EComponent(Ptr<E2EComponent> component)
{
    if (not m_components.insert({component->GetComponentId(), component}).second)
    {
        NS_ABORT_MSG("Component '" << component->GetId() << "' was already added to '"
            << GetId() << "'");
    }
}

std::vector<std::string_view>
E2EComponent::SplitIdPath(std::string_view id)
{
    std::vector<std::string_view> idPath;
    std::string_view path {id};

    while (path.size() > 0)
    {
        std::string_view part {path};
        auto pos = part.find('/');
        if (pos == 0)
        {
            path.remove_prefix(1);
            continue;
        }
        else if (pos == std::string_view::npos)
        {
            path.remove_prefix(path.size());
        }
        else
        {
            part.remove_suffix(part.size() - pos);
            path.remove_prefix(pos + 1);
        }
        idPath.push_back(part);
    }

    return idPath;
}

} // namespace ns3