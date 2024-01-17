
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

NS_LOG_COMPONENT_DEFINE ("SimbricksNicIfExample");

int main (int argc, char *argv[]){

    Time::SetResolution (Time::Unit::PS);
    std::string uxSocketPath;
    std::string shmPath;
    uint64_t syncDelay;
    uint64_t pollDelay;
    uint64_t ethLatency;
    int sync;
    bool sync_mode;

    bool verbose = false;


    CommandLine cmd (__FILE__);
    cmd.AddValue ("verbose", "turn on log components", verbose);
    cmd.AddValue ("uxsoc", "set unix socket path",
        uxSocketPath);
    cmd.AddValue ("shm", "set shared memory path",
        shmPath);
    cmd.AddValue ("syncDelay", "set syncDelay",
        syncDelay);
    cmd.AddValue ("pollDelay", "set pollDelay",
        pollDelay);
    cmd.AddValue ("ethLatency", "set ethLatency",
        ethLatency);
    cmd.AddValue ("sync", "set sync",
        sync);
    cmd.AddValue ("sync_mode", "set sync_mode",
        sync_mode); 

    cmd.Parse (argc, argv);


    if (verbose)
    {
        LogComponentEnable ("PacketSocketServer", LOG_LEVEL_ALL);
        LogComponentEnable ("PacketSocketClient", LOG_LEVEL_ALL);
        LogComponentEnable ("SimbricksNetDeviceNicIf", LOG_LEVEL_ALL);
        LogComponentEnable ("SimbricksAdapterNicIf", LOG_LEVEL_ALL);
        //LogComponentEnable ("SimbricksNicIfExample", LOG_LEVEL_ALL);
        LogComponentEnableAll(LOG_PREFIX_NODE);
    }

    //NS_LOG_INFO("NicifTowHost::uxsoc = " << uxSocketPath.c_str());

    NodeContainer nodes;
    nodes.Create(1);

    ns3::PacketMetadata::Enable();

    PacketSocketHelper packetSocket;
    
    // give packet socket powers to nodes.
    packetSocket.Install (nodes);

    Ptr<simbricks::SimbricksNetDevice> txDev;
    txDev = CreateObject<simbricks::SimbricksNetDevice> ();
    txDev->SetAttribute("UnixSocket", StringValue(uxSocketPath.c_str()));
    //txDev->SetAttribute("Shm", StringValue(shmPath));
    txDev->SetAttribute("SyncDelay", TimeValue(PicoSeconds(syncDelay)));
    txDev->SetAttribute("PollDelay", TimeValue(PicoSeconds(pollDelay)));
    txDev->SetAttribute("EthLatency", TimeValue(PicoSeconds(ethLatency)));
    txDev->SetAttribute("Sync", IntegerValue(sync));
    txDev->SetAttribute("SyncMode", BooleanValue(sync_mode));
    txDev->SetAttribute("Listen", BooleanValue(true));

    nodes.Get (0)->AddDevice (txDev);
    txDev->SetNode (nodes.Get (0));


    
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice (txDev->GetIfIndex ());
    //set destination address 
    Ptr<SimpleNetDevice> rxDev;
    rxDev = CreateObject<SimpleNetDevice> ();
    Mac48Address dest_address("01:02:03:04:05:06");
    rxDev->SetAddress(dest_address);
    //Mac48Address dest_address("01:02:03:04:05:06");
    //Address ad = dest_address.ConvertTo();

    socketAddr.SetPhysicalAddress (rxDev->GetAddress());
    socketAddr.SetProtocol (1);

    
    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
    client->SetRemote (socketAddr);
    client->SetAttribute("Interval", TimeValue (MicroSeconds (1.0)));
    client->SetAttribute("MaxPackets", UintegerValue(20));
    nodes.Get (0)->AddApplication (client);

    txDev->Start();

    NS_LOG_INFO ("Run Emulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
