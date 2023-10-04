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

#ifndef E2E_CC_H
#define E2E_CC_H

#include "ns3/object.h"
#include "ns3/object-factory.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup e2e-cc Description of the e2e-cc
 */

namespace ns3
{

// Each class should be documented using Doxygen,
// and have an \ingroup e2e-cc directive

/* ... */
class E2EConfig
{
  private:
    using args_type = std::unordered_map<std::string_view, std::string_view>;

  public:
    E2EConfig(const std::string &args);

    const args_type& GetArgs() const;
    std::optional<std::string_view> Find(std::string_view key) const;

    template<typename T, typename U>
    bool SetAttrIfContained(Ptr<Object> obj, std::string_view mapKey,
        const std::string& attributeKey) const;

    template<typename T, typename U>
    bool SetFactoryIfContained(ObjectFactory &factory, std::string_view mapKey,
        const std::string& attributeKey) const;

    static int64_t ConvertArgToInteger(const std::string &arg);
    static uint64_t ConvertArgToUInteger(const std::string &arg);

  private:
    std::string m_rawArgs;
    args_type m_parsedArgs;

    void SplitArgs();
};

template <typename T, typename U>
inline bool
E2EConfig::SetAttrIfContained(Ptr<Object> obj,
                              std::string_view mapKey,
                              const std::string& attributeKey) const
{
    if (auto it = m_parsedArgs.find(mapKey); it != m_parsedArgs.end())
    {
        if constexpr (std::is_same_v<U, std::string>)
        {
            obj->SetAttribute(attributeKey, T(std::string(it->second)));
        }
        else if constexpr (std::is_same_v<U, int>)
        {
            int64_t value {ConvertArgToInteger(std::string(it->second))};
            obj->SetAttribute(attributeKey, T(value));
        }
        else if constexpr (std::is_same_v<U, unsigned>)
        {
            uint64_t value {ConvertArgToUInteger(std::string(it->second))};
            obj->SetAttribute(attributeKey, T(value));
        }
        else if constexpr (std::is_same_v<U, InetSocketAddress>)
        {
            std::string_view address {it->second};
            std::string_view portString {it->second};

            auto pos {address.find(':')};
            NS_ABORT_MSG_IF(pos == std::string_view::npos, "Invalid address '" << address << "'");

            address.remove_suffix(address.size() - pos);
            portString.remove_prefix(pos + 1);

            auto port {ConvertArgToUInteger(std::string(portString))};
            NS_ABORT_MSG_IF(port > 65535, "Port '" << port << "' is out of range");
            obj->SetAttribute(attributeKey, T(U(std::string(address).c_str(), port)));
        }
        else
        {
            obj->SetAttribute(attributeKey, T(U(std::string(it->second))));
        }
        return true;
    }
    return false;
}

template <typename T, typename U>
inline bool
E2EConfig::SetFactoryIfContained(ObjectFactory& factory,
                                 std::string_view mapKey,
                                 const std::string& attributeKey) const
{
    if (auto it = m_parsedArgs.find(mapKey); it != m_parsedArgs.end())
    {
        if constexpr (std::is_same_v<U, std::string>)
        {
            factory.Set(attributeKey, T(std::string(it->second)));
        }
        else if constexpr (std::is_same_v<U, int>)
        {
            int64_t value {ConvertArgToInteger(std::string(it->second))};
            factory.Set(attributeKey, T(value));
        }
        else if constexpr (std::is_same_v<U, unsigned>)
        {
            uint64_t value {ConvertArgToUInteger(std::string(it->second))};
            factory.Set(attributeKey, T(value));
        }
        else if constexpr (std::is_same_v<U, InetSocketAddress>)
        {
            std::string_view address {it->second};
            std::string_view portString {it->second};

            auto pos {address.find(':')};
            NS_ABORT_MSG_IF(pos == std::string_view::npos, "Invalid address '" << address << "'");

            address.remove_suffix(address.size() - pos);
            portString.remove_prefix(pos + 1);

            auto port {ConvertArgToUInteger(std::string(portString))};
            NS_ABORT_MSG_IF(port > 65535, "Port '" << port << "' is out of range");
            factory.Set(attributeKey, T(U(std::string(address).c_str(), port)));
        }
        else
        {
            factory.Set(attributeKey, T(U(std::string(it->second))));
        }
        return true;
    }
    return false;
}

/* ... */
class E2EConfigParser
{
  public:
    E2EConfigParser() = default;
    
    void ParseArguments(int argc, char *argv[]);

    const std::vector<E2EConfig>& GetTopologyArgs();
    const std::vector<E2EConfig>& GetHostArgs();
    const std::vector<E2EConfig>& GetApplicationArgs();
    const std::vector<E2EConfig>& GetProbeArgs();
    const std::vector<E2EConfig>& GetGlobalArgs();

  private:
    std::vector<E2EConfig> m_topologies;
    std::vector<E2EConfig> m_hosts;
    std::vector<E2EConfig> m_applications;
    std::vector<E2EConfig> m_probes;
    std::vector<E2EConfig> m_globals;

    static bool AddConfig(std::vector<E2EConfig> *configs, const std::string &args);
};

} // namespace ns3

#endif /* E2E_CC_H */
