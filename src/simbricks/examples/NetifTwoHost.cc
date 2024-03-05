
// Network topology
//
//       n0(NicIf)     n1(NetIf)
//       |               |
//       =================
//         SimbricksChannel
//
// - Packets flows from n0 to n1
//
// This example shows how to use the PacketSocketServer and PacketSocketClient
// to send non-IP packets over a SimpleNetDevice

#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/simbricks-netdev.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimbricksNetIfExample");

int main (int argc, char *argv[]){

    Time::SetResolution (Time::Unit::PS);
    std::string uxSocketPath;
    uint64_t syncDelay;
    uint64_t pollDelay;
    uint64_t ethLatency;
    int sync;

    bool verbose = false;


    CommandLine cmd (__FILE__);
    cmd.AddValue ("verbose", "turn on log components", verbose);
    cmd.AddValue ("uxsoc", "set unix socket path",
        uxSocketPath);

    cmd.AddValue ("syncDelay", "set syncDelay",
        syncDelay);
    cmd.AddValue ("pollDelay", "set pollDelay",
        pollDelay);
    cmd.AddValue ("ethLatency", "set ethLatency",
        ethLatency);
    cmd.AddValue ("sync", "set sync",
        sync);

    cmd.Parse (argc, argv);


    if (verbose)
    {
        LogComponentEnable ("PacketSocketServer", LOG_LEVEL_ALL);
        LogComponentEnable ("PacketSocketClient", LOG_LEVEL_ALL);
        LogComponentEnable ("SimbricksNetDevice", LOG_LEVEL_ALL);
        LogComponentEnable ("SimbricksAdapter", LOG_LEVEL_ALL);
        LogComponentEnable ("SimbricksNetIfExample", LOG_LEVEL_ALL);
        LogComponentEnableAll(LOG_PREFIX_NODE);
    }

    //NS_LOG_INFO("NicifTowHost::uxsoc = " << uxSocketPath.c_str());

    NodeContainer nodes;
    nodes.Create(1);

    ns3::PacketMetadata::Enable();

    PacketSocketHelper packetSocket;
    
    // give packet socket powers to nodes.
    packetSocket.Install (nodes);

    Ptr<simbricks::SimbricksNetDevice> rxDev;
    rxDev = CreateObject<simbricks::SimbricksNetDevice> ();
    rxDev->SetAttribute("UnixSocket", StringValue(uxSocketPath));
    rxDev->SetAttribute("SyncDelay", TimeValue(PicoSeconds(syncDelay)));
    rxDev->SetAttribute("PollDelay", TimeValue(PicoSeconds(pollDelay)));
    rxDev->SetAttribute("EthLatency", TimeValue(PicoSeconds(ethLatency)));
    rxDev->SetAttribute("Sync", IntegerValue(sync));

    nodes.Get (0)->AddDevice (rxDev);
    rxDev->SetNode (nodes.Get (0));


    
    Ptr<SimpleNetDevice> txDev;
    PacketSocketAddress socketAddr;
    txDev = CreateObject<SimpleNetDevice> ();
    socketAddr.SetSingleDevice (txDev->GetIfIndex ());
    Mac48Address dest_address("01:02:03:04:05:06");
    txDev->SetAddress(dest_address);
    socketAddr.SetPhysicalAddress (txDev->GetAddress());

    socketAddr.SetProtocol (1);

    
    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
    server->SetLocal (socketAddr);
    nodes.Get (0)->AddApplication (server);

    rxDev->Start();

    NS_LOG_INFO ("Run Emulation.");

    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
