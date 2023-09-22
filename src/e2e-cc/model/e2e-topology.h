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

#ifndef E2E_TOPOLOGY_H
#define E2E_TOPOLOGY_H

#include "e2e-host.h"
#include "e2e-config.h"
#include "e2e-component.h"

#include "ns3/network-module.h"

namespace ns3
{

/* ... */

class E2ETopology : public E2EComponent
{
  public:
    E2ETopology(const E2EConfig& config);

    static Ptr<E2ETopology> CreateTopology(const E2EConfig& config);

    virtual void AddHost(Ptr<E2EHost> host) = 0;
};

class E2EDumbbellTopology : public E2ETopology
{
  public:
    E2EDumbbellTopology(const E2EConfig& config);

    void AddHost(Ptr<E2EHost> host) override;

  private:
    NodeContainer m_switches;
    NetDeviceContainer m_switchDevices;
    NetDeviceContainer m_bottleneckDevices;
};

} // namespace ns3

#endif /* E2E_TOPOLOGY_H */
