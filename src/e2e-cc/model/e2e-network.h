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

#ifndef E2E_NETWORK_H
#define E2E_NETWORK_H

#include "e2e-config.h"
#include "e2e-component.h"

#include "ns3/network-module.h"
#include "ns3/simbricks-module.h"

namespace ns3
{

/* ... */
class E2ENetwork : public E2EComponent
{
  public:
    E2ENetwork(const E2EConfig &config);

    static Ptr<E2ENetwork> CreateNetwork(const E2EConfig& config);

    Ptr<NetDevice> GetNetDevice();

  protected:
    Ptr<NetDevice> m_netDevice;
};

class E2ENetworkNetIf : public E2ENetwork
{
  public:
    E2ENetworkNetIf(const E2EConfig &config);
};

class E2ENetworkNicIf : public E2ENetwork
{
  public:
    E2ENetworkNicIf(const E2EConfig &config);
};

class E2ENetworkTrunk : public E2EComponent
{
  public:
    E2ENetworkTrunk(const E2EConfig &config);

    Ptr<NetDevice> AddDevice(int id);

  private:
    Ptr<simbricks::SimbricksTrunk> m_trunk;
    int m_current_id = -1;
};

class E2ENetworkTrunkDevice : public E2ENetwork
{
  public:
    E2ENetworkTrunkDevice(const E2EConfig &config, Ptr<E2EComponent> root, int64_t orderId);
};

} // namespace ns3

#endif /* E2E_NETWORK_H */
