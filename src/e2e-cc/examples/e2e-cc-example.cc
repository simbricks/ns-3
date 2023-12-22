#include "ns3/core-module.h"
#include "ns3/e2e-cc-module.h"
#include "ns3/internet-module.h"

#include <iostream>

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
