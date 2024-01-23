#include "ns3/core-module.h"
#include "ns3/e2e-cc-module.h"
#include "ns3/internet-module.h"

#include <iostream>
#include <queue>

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;

int
main(int argc, char* argv[])
{
    LogComponentEnable("E2EConfig", LOG_LEVEL_WARN);
    LogComponentEnable("E2EComponent", LOG_LEVEL_WARN);
    LogComponentEnable("E2ETopology", LOG_LEVEL_WARN);
    LogComponentEnable("E2EHost", LOG_LEVEL_WARN);
    LogComponentEnable("E2EApplication", LOG_LEVEL_WARN);

    Time::SetResolution (Time::Unit::PS);

    E2EConfigParser configParser {};
    configParser.ParseArguments(argc, argv);

    if (configParser.GetGlobalArgs().size() == 1)
    {
        auto& globalConfig = configParser.GetGlobalArgs()[0];
        if (auto stopTime {globalConfig.Find("StopTime")}; stopTime)
        {
            Simulator::Stop(Time(std::string(*stopTime)));
        }
        if (auto macStart {globalConfig.Find("MACStart")}; macStart) {
            Mac48Address::SetAllocationIndex(
              std::stoull(std::string(*macStart)));
        }
    }

    E2EConfig rootConfig("");
    Ptr<E2EComponent> root = Create<E2EComponent>(rootConfig);

    auto& topologyNodeConfigs = configParser.GetTopologyNodeArgs();
    for (auto& config : topologyNodeConfigs)
    {
        root->AddE2EComponent(E2ETopologyNode::CreateTopologyNode(config));
    }

    auto& topologyChannelConfigs = configParser.GetTopologyChannelArgs();
    for (auto& config : topologyChannelConfigs)
    {
        Ptr<E2ETopologyChannel> channel = E2ETopologyChannel::CreateTopologyChannel(config);
        channel->Connect(root);
        root->AddE2EComponent(channel);
    }

    auto& hostConfigs = configParser.GetHostArgs();
    for (auto& config : hostConfigs)
    {
        Ptr<E2EHost> host = E2EHost::CreateHost(config);
        auto topology = root->GetE2EComponentParent<E2ETopologyNode>(host->GetIdPath());
        NS_ABORT_MSG_UNLESS(topology, "Topology for node '" << host->GetId() << "' not found");
        (*topology)->AddHost(host);
    }

    // store trunk devices in a priority queue and create them in the
    // right order after all trunks exist
    using elem_t = std::pair<int, const E2EConfig*>;
    auto cmp = [](elem_t left, elem_t right) { return left.first > right.first; };
    std::priority_queue<elem_t, std::vector<elem_t>, decltype(cmp)> trunkDevices(cmp);

    auto& networkConfigs = configParser.GetNetworkArgs();
    for (auto& config : networkConfigs)
    {
        auto type = config.Find("Type");
        NS_ABORT_MSG_UNLESS(type, "Network component has no type");

        if (*type == "Trunk")
        {
            Ptr<E2ENetworkTrunk> trunk = Create<E2ENetworkTrunk>(config);
            root->AddE2EComponent(trunk);
        }
        else if (*type == "TrunkDevice")
        {
            auto orderIdStr = config.Find("OrderId");
            NS_ABORT_MSG_UNLESS(orderIdStr, "Trunk device does not contain an order id");
            auto orderId = config.ConvertArgToInteger(std::string(*orderIdStr));
            trunkDevices.emplace(orderId, &config);
        }
        else
        {
            Ptr<E2ENetwork> network = E2ENetwork::CreateNetwork(config);
            auto topology = root->GetE2EComponentParent<E2ETopologyNode>(network->GetIdPath());
            NS_ABORT_MSG_UNLESS(topology,
                "Topology for node '" << network->GetId() << "' not found");
            (*topology)->AddNetwork(network);
        }
    }

    // create the trunk devices
    while (not trunkDevices.empty())
    {
        auto devConf = trunkDevices.top();
        trunkDevices.pop();
        auto device = Create<E2ENetworkTrunkDevice>(*devConf.second, root, devConf.first);
        auto topology = root->GetE2EComponentParent<E2ETopologyNode>(device->GetIdPath());
        NS_ABORT_MSG_UNLESS(topology,
            "Topology for node '" << device->GetId() << "' not found");
        (*topology)->AddNetwork(device);
    }

    auto& applicationConfigs = configParser.GetApplicationArgs();
    for (auto& config : applicationConfigs)
    {
        Ptr<E2EApplication> application = E2EApplication::CreateApplication(config);
        auto host = root->GetE2EComponentParent<E2EHost>(application->GetIdPath());
        NS_ABORT_MSG_UNLESS(host,
            "Host for application '" << application->GetId() << "' not found");
        (*host)->AddApplication(application);
    }

    for (auto& config : configParser.GetProbeArgs())
    {
        auto id = config.Find("Id");
        NS_ABORT_MSG_UNLESS(id, "Probe has no id");

        auto component = root->GetE2EComponentParent<E2EComponent>(*id);
        NS_ABORT_MSG_UNLESS(component, "Component for probe '" << *id << "' not found");

        (*component)->AddProbe(config);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
