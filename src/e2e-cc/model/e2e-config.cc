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

#include <filesystem>

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
    std::string configFile;
    CommandLine cmd (__FILE__);
    cmd.AddValue("TopologyNode", "Add a topology node to the simulation",
        MakeBoundCallback(AddConfig, &m_topologyNodes));
    cmd.AddValue("TopologyChannel", "Add a topology channel to the simulation",
        MakeBoundCallback(AddConfig, &m_topologyChannels));
    cmd.AddValue("Host", "Add a host to the simulation", MakeBoundCallback(AddConfig, &m_hosts));
    cmd.AddValue("App", "Add an application to the simulation",
        MakeBoundCallback(AddConfig, &m_applications));
    cmd.AddValue("Probe", "Add a probe to the simulation", MakeBoundCallback(AddConfig, &m_probes));
    cmd.AddValue("Global", "Add global options", MakeBoundCallback(AddConfig, &m_globals));
    cmd.AddValue("ConfigFile", "A file that contains command line options", configFile);
    cmd.Parse(argc, argv);

    if (not configFile.empty())
    {
        std::filesystem::path p(configFile);
        //check if file exists
        NS_ABORT_MSG_UNLESS(std::filesystem::exists(p) and std::filesystem::is_regular_file(p),
            "Config file " << configFile << " does not exist or is not a file");
        
        constexpr int BUFFER_SIZE = 128;
        char buffer[BUFFER_SIZE];
        std::vector<std::string> args;
        // the first argument gets discarded by cmd.Parse since it expects it to be the program name
        args.push_back("");
        std::ifstream file(p);
        std::ostringstream argBuffer;
        char currentDelimiter;
        bool quoted = false;
        bool skipWhitespace = false;

        while (not file.eof())
        {
            file.read(buffer, BUFFER_SIZE);
            int readChars = file.gcount();
            for (int i = 0; i < readChars; ++i)
            {
                if (quoted)
                {
                    if (buffer[i] == currentDelimiter)
                    {
                        // end quoting and continue with next char
                        quoted = false;
                    }
                    else
                    {
                        argBuffer << buffer[i];
                    }
                }
                else if (buffer[i] == ' ' or buffer[i] == '\n')
                {
                    if (skipWhitespace)
                    {
                        continue;
                    }
                    // finish last argument
                    args.push_back(argBuffer.str());
                    argBuffer.str("");
                    skipWhitespace = true;
                }
                else if (buffer[i] == '\'' or buffer[i] == '"')
                {
                    skipWhitespace = false;
                    currentDelimiter = buffer[i];
                    quoted = true;
                }
                else
                {
                    skipWhitespace = false;
                    argBuffer << buffer[i];
                }
            }
        }

        // there is possibly one last arg sitting in argBuffer
        if (auto lastArg = argBuffer.str(); not lastArg.empty())
        {
            args.push_back(lastArg);
        }
        
        file.close();
        cmd.Parse(args);
    }

    NS_ABORT_MSG_IF(m_globals.size() > 1, "Global options should be given only once");
}

const std::vector<E2EConfig>&
E2EConfigParser::GetTopologyNodeArgs()
{
    return m_topologyNodes;
}

const std::vector<E2EConfig>&
E2EConfigParser::GetTopologyChannelArgs()
{
    return m_topologyChannels;
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
E2EConfigParser::GetProbeArgs()
{
    return m_probes;
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
