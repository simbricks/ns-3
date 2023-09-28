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

#include "e2e-config.h"

#include "ns3/core-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("E2EConfig");

E2EConfig::E2EConfig(const std::string &args) : m_rawArgs{args}
{
    SplitArgs();
}

const E2EConfig::args_type&
E2EConfig::GetArgs() const
{
    return m_parsedArgs;
}

std::optional<std::string_view>
E2EConfig::Find(std::string_view key) const
{
    if (auto it = m_parsedArgs.find(key); it != m_parsedArgs.end())
    {
        return it->second;
    }
    return {};
}

int64_t
E2EConfig::ConvertArgToInteger(const std::string& arg)
{
    int64_t value;
    std::size_t pos{};
    try
    {
        value = std::stol(arg, &pos);
    }
    catch (std::invalid_argument const&)
    {
        NS_ABORT_MSG("unable to convert input '" << arg << "' into integer");
    }
    catch (std::out_of_range const&)
    {
        NS_ABORT_MSG("input '" << arg << "' out of range");
    }

    if (pos != arg.size())
    {
        NS_LOG_WARN("input '" << arg << "' contains non-numeric characters that were ignored");
    }

    return value;
}

uint64_t
E2EConfig::ConvertArgToUInteger(const std::string& arg)
{
    uint64_t value;
    std::size_t pos{};
    try
    {
        value = std::stoul(arg, &pos);
    }
    catch (std::invalid_argument const&)
    {
        NS_ABORT_MSG("unable to convert input '" << arg << "' into integer");
    }
    catch (std::out_of_range const&)
    {
        NS_ABORT_MSG("input '" << arg << "' out of range");
    }

    if (pos != arg.size())
    {
        NS_LOG_WARN("input '" << arg << "' contains non-numeric characters that were ignored");
    }

    return value;
}

void
E2EConfig::SplitArgs()
{
    std::string_view arg_view {m_rawArgs};

    while (arg_view.size() > 0)
    {
        // extract key
        std::string_view key {arg_view};
        auto pos = key.find(':');
        NS_ABORT_MSG_IF(pos == std::string_view::npos,
            "Invalid argument format: key without value (" << key << ")");
        key.remove_suffix(key.size() - pos);
        arg_view.remove_prefix(pos + 1);

        // extract value
        std::string_view value {arg_view};
        pos = value.find(';');
        if (pos != std::string_view::npos)
        {
            value.remove_suffix(value.size() - pos);
            arg_view.remove_prefix(pos + 1);
        }
        else
        {
            arg_view.remove_prefix(arg_view.size());
        }

        m_parsedArgs.emplace(key, value);
    }
}

void
E2EConfigParser::ParseArguments(int argc, char *argv[])
{
    CommandLine cmd (__FILE__);
    cmd.AddValue("Topology", "Set the topology of the simulation",
        MakeBoundCallback(AddConfig, &m_topologies));
    cmd.AddValue("Host", "Add a host to the simulation", MakeBoundCallback(AddConfig, &m_hosts));
    cmd.AddValue("App", "Add an application to the simulation",
        MakeBoundCallback(AddConfig, &m_applications));
    cmd.AddValue("Global", "Add global options", MakeBoundCallback(AddConfig, &m_globals));

    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(m_topologies.size() > 1,
        "Currently only one topology is supported per experiment");

    NS_ABORT_MSG_IF(m_globals.size() > 1, "Global options should be given only once");
}

const std::vector<E2EConfig>&
E2EConfigParser::GetTopologyArgs()
{
    return m_topologies;
}

const std::vector<E2EConfig>&
E2EConfigParser::GetHostArgs()
{
    return m_hosts;
}

const std::vector<E2EConfig>&
E2EConfigParser::GetApplicationArgs()
{
    return m_applications;
}

const std::vector<E2EConfig>&
E2EConfigParser::GetGlobalArgs()
{
    return m_globals;
}

bool
E2EConfigParser::AddConfig(std::vector<E2EConfig> *config, const std::string &args)
{
    config->emplace_back(args);
    return true;
}

} // namespace ns3
