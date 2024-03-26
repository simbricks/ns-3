#include "ns3/e2e-cc-module.h"

#include <queue>

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;

/** Mapping of log level text names to values. (copied from log.cc) */
const std::map<std::string, ns3::LogLevel> LOG_LABEL_LEVELS = {
    // clang-format off
        {"none",           ns3::LOG_NONE},
        {"error",          ns3::LOG_ERROR},
        {"level_error",    ns3::LOG_LEVEL_ERROR},
        {"warn",           ns3::LOG_WARN},
        {"level_warn",     ns3::LOG_LEVEL_WARN},
        {"debug",          ns3::LOG_DEBUG},
        {"level_debug",    ns3::LOG_LEVEL_DEBUG},
        {"info",           ns3::LOG_INFO},
        {"level_info",     ns3::LOG_LEVEL_INFO},
        {"function",       ns3::LOG_FUNCTION},
        {"level_function", ns3::LOG_LEVEL_FUNCTION},
        {"logic",          ns3::LOG_LOGIC},
        {"level_logic",    ns3::LOG_LEVEL_LOGIC},
        {"all",            ns3::LOG_ALL},
        {"level_all",      ns3::LOG_LEVEL_ALL},
        {"func",           ns3::LOG_PREFIX_FUNC},
        {"prefix_func",    ns3::LOG_PREFIX_FUNC},
        {"time",           ns3::LOG_PREFIX_TIME},
        {"prefix_time",    ns3::LOG_PREFIX_TIME},
        {"node",           ns3::LOG_PREFIX_NODE},
        {"prefix_node",    ns3::LOG_PREFIX_NODE},
        {"level",          ns3::LOG_PREFIX_LEVEL},
        {"prefix_level",   ns3::LOG_PREFIX_LEVEL},
        {"prefix_all",     ns3::LOG_PREFIX_ALL}
    // clang-format on
};

void
PrintProgress(Time interval, Time end)
{
    NS_LOG_UNCOND("current time: " << Simulator::Now().GetMilliSeconds() << "ms");
    if (Simulator::Now() + interval < end)
    {
        Simulator::Schedule(interval, PrintProgress, interval, end);
    }
}

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

    auto& loggingConfigs = configParser.GetLoggingArgs();
    for (auto& config : loggingConfigs)
    {
        for (auto& entry : config)
        {
            entry.second.processed = true;
            std::string_view levels = entry.second.value;
            std::string logComponent {entry.first};

            while (not levels.empty())
            {
                auto pos = levels.find('|');
                std::string_view level;
                if (pos != std::string_view::npos)
                {
                    level = levels.substr(0, pos);
                    levels.remove_prefix(pos + 1);
                }
                else
                {
                    level = levels;
                    levels = std::string_view();
                }
                auto it = LOG_LABEL_LEVELS.find(std::string(level));
                NS_ABORT_MSG_IF(it == LOG_LABEL_LEVELS.end(),
                    "Log level " << level << " not found");

                if (logComponent == "All")
                {
                    LogComponentEnableAll(it->second);
                }
                else
                {
                    LogComponentEnable(logComponent, it->second);
                }
            }
        }
    }

    if (configParser.GetGlobalArgs().size() == 1)
    {
        auto& globalConfig = configParser.GetGlobalArgs()[0];
        if (auto stopTime {globalConfig.Find("StopTime")}; stopTime)
        {
            Simulator::Stop(Time(std::string((*stopTime).value)));
            (*stopTime).processed = true;
        }
        if (auto macStart {globalConfig.Find("MACStart")}; macStart) {
            Mac48Address::SetAllocationIndex(
              std::stoull(std::string((*macStart).value)));
            (*macStart).processed = true;
        }
        if (auto progress {globalConfig.Find("Progress")}; progress)
        {
            auto pos = progress->value.find(',');
            NS_ABORT_MSG_IF(pos == std::string_view::npos, "Invalid progress value");
            Time interval(std::string(progress->value.substr(0, pos)));
            Time end(std::string(progress->value.substr(pos + 1)));
            Simulator::ScheduleNow(PrintProgress, interval, end);
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

        if ((*type).value == "Trunk")
        {
            Ptr<E2ENetworkTrunk> trunk = Create<E2ENetworkTrunk>(config);
            root->AddE2EComponent(trunk);
        }
        else if ((*type).value == "TrunkDevice")
        {
            auto orderIdStr = config.Find("OrderId");
            NS_ABORT_MSG_UNLESS(orderIdStr, "Trunk device does not contain an order id");
            (*orderIdStr).processed = true;
            auto orderId = E2EConfig::ConvertArgToInteger(std::string((*orderIdStr).value));
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

        auto component = root->GetE2EComponentParent<E2EComponent>((*id).value);
        NS_ABORT_MSG_UNLESS(component, "Component for probe '" << (*id).value << "' not found");

        (*component)->AddProbe(config);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
