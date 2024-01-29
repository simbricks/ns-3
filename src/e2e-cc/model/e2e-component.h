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

#ifndef E2E_COMPONENT_H
#define E2E_COMPONENT_H

#include "e2e-config.h"

namespace ns3
{

/* ... */
class E2EComponent : public SimpleRefCount<E2EComponent>
{
  public:
    E2EComponent(const E2EConfig &config);
    virtual ~E2EComponent() = default;

    std::string_view GetId() const;
    std::string_view GetComponentId() const;
    std::string_view GetType() const;
    const E2EConfig& GetConfig() const;
    const std::vector<std::string_view>& GetIdPath() const;

    virtual void AddProbe(const E2EConfig& config);

    void AddE2EComponent(Ptr<E2EComponent> component);
    template<typename T>
    std::optional<Ptr<T>> GetE2EComponent(const std::vector<std::string_view>& idPath);
    template<typename T>
    std::optional<Ptr<T>> GetE2EComponent(std::string_view idPath);
    template<typename T>
    std::optional<Ptr<T>> GetE2EComponentParent(const std::vector<std::string_view>& idPath);
    template<typename T>
    std::optional<Ptr<T>> GetE2EComponentParent(std::string_view idPath);

  protected:
    const E2EConfig &m_config;

  private:
    std::string_view m_id;
    std::string_view m_type;
    std::vector<std::string_view> m_idPath;
    std::unordered_map<std::string_view, Ptr<E2EComponent>> m_components;

    static std::vector<std::string_view> SplitIdPath(std::string_view id);
};

template<typename T>
inline std::optional<Ptr<T>>
E2EComponent::GetE2EComponent(const std::vector<std::string_view>& idPath)
{
    if (idPath.empty())
    {
        return {};
    }

    Ptr<E2EComponent> component;
    if (auto it {m_components.find(idPath[0])}; it != m_components.end())
    {
        component = it->second;
    }
    else
    {
        return {};
    }

    for (std::size_t i = 1; i < idPath.size(); ++i)
    {
        if (auto it {component->m_components.find(idPath[i])}; it != component->m_components.end())
        {
            component = it->second;
        }
        else
        {
            return {};
        }

    }

    // component should now contain what we are looking for
    if constexpr (std::is_same_v<T, E2EComponent>)
    {
        return component;
    }
    else
    {
        Ptr<T> casted {DynamicCast<T>(component)};
        if (casted)
        {
            return casted;
        }
        else
        {
            //Todo: is it possible to use logging here?
            //NS_LOG_ERROR("Component found but with different type");
            std::cerr << "Component found but with different type\n";
            return {};
        }
    }
}

template<typename T>
inline std::optional<Ptr<T>>
E2EComponent::GetE2EComponent(std::string_view idPath)
{
    auto splittedIdPath {SplitIdPath(idPath)};
    return GetE2EComponent<T>(splittedIdPath);
}

template<typename T>
inline std::optional<Ptr<T>>
E2EComponent::GetE2EComponentParent(const std::vector<std::string_view>& idPath)
{
    //Todo: for idPath.size()==1 maybe return something like DynamicCast<T>(Ptr<E2EComponent>(this))
    if (idPath.size() < 2)
    {
        return {};
    }

    Ptr<E2EComponent> component;
    if (auto it {m_components.find(idPath[0])}; it != m_components.end())
    {
        component = it->second;
    }
    else
    {
        return {};
    }

    for (std::size_t i = 1; i < idPath.size() - 1; ++i)
    {
        if (auto it {component->m_components.find(idPath[i])}; it != component->m_components.end())
        {
            component = it->second;
        }
        else
        {
            return {};
        }

    }

    // component should now contain what we are looking for
    if constexpr (std::is_same_v<T, E2EComponent>)
    {
        return component;
    }
    else
    {
        Ptr<T> casted {DynamicCast<T>(component)};
        if (casted)
        {
            return casted;
        }
        else
        {
            //Todo: is it possible to use logging here?
            //NS_LOG_ERROR("Component found but with different type");
            std::cerr << "Component found but with different type\n";
            return {};
        }
    }
}

template<typename T>
inline std::optional<Ptr<T>>
E2EComponent::GetE2EComponentParent(std::string_view idPath)
{
    auto splittedIdPath {SplitIdPath(idPath)};
    return GetE2EComponentParent<T>(splittedIdPath);
}

} // namespace ns3

#endif /* E2E_COMPONENT_H */