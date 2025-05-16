#include <sstream>
#include <iomanip>
#include <string>
#include <fstream>
#include <iostream>

#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/simbricks-netdev.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/pcap-file-wrapper.h"
#include "ns3/trace-helper.h"
#include "ns3/flow-monitor-module.h"

#define START 0.0
#define END 5
#define NUM_STEPS 20
#define HEAD_ROOM 1

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("FCT_FatTree_Example");

struct DetailHostPair {
    int num_detail_ser;
    int num_detail_cli;
};
struct HostPos {
    int pod;
    int rack;
    int host;
};
std::vector<std::string> simbricksPortPaths;
std::map<Ipv4Address, int> received_bytes; // Map source IP to received bytes
std::map<Ipv4Address, int> rand_delay;
int flow_size = 2; // in MB

void inc_address_base(char* array, int &sub){
	std::string ip1 = "10.";
	std::string ip2 = std::to_string(sub/256);
	std::string ip3 = std::to_string(sub%256);
	std::string ip4 = ".0";
	ip1 = ip1+ip2+"."+ip3+ip4;
	delete [] array;
	array = new char[ip1.size()];
	std::strcpy(array, ip1.c_str());
	sub++;
}

bool
AddSimbricksPort(const std::string& arg)
{
    simbricksPortPaths.push_back(arg);
    return true;
}
void
PrintClientStart(){
    NS_LOG_INFO("Client started at " << Simulator::Now().GetMicroSeconds() << " usec");
}

void
PrintSimProgress()
{
    float step = (END - START) / NUM_STEPS;

    NS_LOG_INFO("Sim. Time: " << Simulator::Now().GetMilliSeconds() << " ms");
    Simulator::Schedule(Seconds(step), &PrintSimProgress);
}

Ipv4Address
node_id_to_ip(uint32_t id)
{
    return Ipv4Address(0x0a000001 + ((id / 256) * 0x00010000) + ((id % 256) * 0x00000100));
}

uint32_t
ip_to_node_id(Ipv4Address ip)
{
    return (ip.Get() >> 8) & 0xffff;
}


void PrintAllNetDeviceIPs() {
    NS_LOG_INFO("Iterating through all nodes and their NetDevices to print IP addresses:");
    
    // Iterate over all nodes
    for (uint32_t nodeId = 0; nodeId < NodeList::GetNNodes(); ++nodeId) {
        Ptr<Node> node = NodeList::GetNode(nodeId);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (!ipv4) {
            NS_LOG_WARN("Node " << nodeId << " does not have an Ipv4 object.");
            continue;
        }
        
        NS_LOG_INFO("Node " << nodeId << " has " << node->GetNDevices() << " NetDevices:");
        
        // Iterate over all NetDevices in the node
        for (uint32_t devId = 0; devId < node->GetNDevices(); ++devId) {
            Ptr<NetDevice> device = node->GetDevice(devId);
            
            // Find the interface index for this NetDevice
            int32_t interfaceIndex = ipv4->GetInterfaceForDevice(device);
            if (interfaceIndex >= 0) {
                // Get the primary IP address of the interface
                Ipv4Address ipAddress = ipv4->GetAddress(interfaceIndex, 0).GetLocal();
                NS_LOG_INFO("  Device " << devId << ": " << device->GetInstanceTypeId()
                << " IP Address: " << ipAddress);
            } else {
                NS_LOG_INFO("  Device " << devId << ": " << device->GetInstanceTypeId()
                << " has no IP address.");
            }
        }
    }
}

struct HostPos GetPodRackHostFromHostIdx(
    int host_idx, int num_pod, int racks_per_pod, int num_hosts_per_rack)
{

    struct HostPos result;
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            for (int k = 0; k < num_hosts_per_rack; k++){
                
                int current_idx = num_pod * racks_per_pod * k + i * racks_per_pod + j;
                if (current_idx == host_idx){
                    result.pod = i;
                    result.rack = j;
                    result.host = k;
                }
            }
        }
    }

    return result;
}

void
log_fct(Ptr<Packet const> packet, const Address& address)
{
    // Extract the source IP address from the packet
    Ipv4Address sourceIp = InetSocketAddress::ConvertFrom(address).GetIpv4();

    // Increment the received bytes for the source IP
    received_bytes[sourceIp] += packet->GetSize();
    uint64_t start_time = rand_delay[sourceIp];

    if (received_bytes[sourceIp] >= flow_size * 1024 * 1024)
    {   
        NS_LOG_INFO("Sink " << "started at: " << start_time << " Completed receiving " << received_bytes[sourceIp] << " Bytes from" << sourceIp << " at "
                            << Simulator::Now().GetMicroSeconds() << " usec" << " FCT: " << Simulator::Now().GetMicroSeconds() - start_time << " usec");
        
    }
}

void
sink(ns3::Ipv4Address add, ns3::Ptr<Node> node)
{
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(add, 8080));
    ApplicationContainer sinkApp = packetSinkHelper.Install(node);
    
    sinkApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&log_fct));
    sinkApp.Start(Seconds(START + HEAD_ROOM));
    sinkApp.Stop(Seconds(END));
}

void
client(ns3::Ipv4Address add, ns3::Ptr<Node> node, int flow_size)
{
    OnOffHelper client("ns3::TcpSocketFactory", InetSocketAddress(add, 8080));
    client.SetAttribute("OnTime",
                        StringValue("ns3::ConstantRandomVariable[Constant=1000000000000]"));
    client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute("DataRate", DataRateValue(DataRate("5000Mbps")));
    client.SetAttribute("MaxBytes", UintegerValue(flow_size * 1024 * 1024));

    client.SetAttribute("PacketSize", UintegerValue(1500));

    ApplicationContainer clientApp = client.Install(node);

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    uv->SetAttribute("Min", DoubleValue(0));
    uv->SetAttribute("Max", DoubleValue(2));
    Time randomDelay = Seconds(uv->GetValue());
    Time startTime = Seconds(START + HEAD_ROOM + randomDelay.GetSeconds());

    rand_delay[node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()] = startTime.GetMicroSeconds();
    NS_LOG_INFO("Client " << node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << " started at " <<  START + HEAD_ROOM + randomDelay.GetSeconds() << " sec");
    
    clientApp.Start(Seconds(START + HEAD_ROOM + randomDelay.GetSeconds()));
    clientApp.Stop(Seconds(END));
}

int
main(int argc, char* argv[])
{

    // LogComponentEnable("PacketSink",(LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    // LogComponentEnable("OnOffApplication",(LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    // LogComponentEnable("Ipv4GlobalRouting", (LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    // LogComponentEnable("SimpleNetDevice", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    // LogComponentEnable("BridgeNetDevice", (LogLevel)(LOG_LEVEL_ALL | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    // LogComponentEnable("CsmaNetDevice", LOG_LEVEL_INFO);
    // LogComponentEnable("Queue", LOG_LEVEL_INFO);
    // LogComponentEnable("TcpSocketBase", (LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
    LogComponentEnable("FCT_FatTree_Example",
                       (LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));

    // LogComponentEnable("SimbricksNetDevice", LOG_LEVEL_ALL);
    // LogComponentEnable("BridgeHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("GlobalRoutingHelper", LOG_LEVEL_ALL);
    

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1460));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    Time::SetResolution(Time::Unit::PS);

    Time linkLatency(NanoSeconds(500));
    DataRate linkRate("40Gb/s");
    DataRate spine_agg_linkRate("200Gb/s");
    DataRate agg_tor_linkRate("100Gb/s");

    double ecnTh = 200000;
    int k_value = 4;
    float detail_host_percent = 0.1;

    // int n_spine_sw = 1;
    // int n_agg_bl = 2;
    // int n_agg_sw = 1;
    // int n_agg_racks = 6;
    // int h_per_rack = 40;

    CommandLine cmd(__FILE__);
    cmd.AddValue("LinkLatency", "Propagation delay through link", linkLatency);
    cmd.AddValue("LinkRate", "Link bandwidth", linkRate);
    cmd.AddValue("EcnTh", "ECN Threshold queue size", ecnTh);
    cmd.AddValue("SimbricksPort",
                 "Add a simbricks ethernet port to the bridge",
                 MakeCallback(&AddSimbricksPort));
    cmd.AddValue("k_value", "Fat tree k value", k_value);
    // cmd.AddValue("n_spine_sw", "Number of spine switches", n_spine_sw);
    // cmd.AddValue("n_agg_bl", "Number of aggregation blocks", n_agg_bl);
    // cmd.AddValue("n_agg_sw", "Number of aggregation switches", n_agg_sw);
    // cmd.AddValue("n_agg_racks", "Number of racks per agg block", n_agg_racks);
    // cmd.AddValue("h_per_rack", "Number of hosts per rack", h_per_rack);

    cmd.AddValue("flow_size", "Flow size in MB, Client send to the Server", flow_size);
    cmd.AddValue("detail_host_percent", "Detail host percent in fat tree", detail_host_percent);

    cmd.Parse(argc, argv);
    
    std::ostringstream out_name_stream;
    out_name_stream << "fct_" << k_value << "_" << detail_host_percent << ".out";
    std::string out_name = out_name_stream.str();

    std::ofstream logFile(out_name.c_str());
    std::clog.rdbuf(logFile.rdbuf());

    int total_hosts = k_value * k_value * k_value / 4;
    int num_detail_hosts = std::round(total_hosts * detail_host_percent);
    if (num_detail_hosts == 0)
    {
        num_detail_hosts = 0;
    }
    else if (num_detail_hosts % 2 != 0)
    {
        num_detail_hosts += 1;
    }
    int starting_host_idx = 0;

    int num_dum_hosts = total_hosts - num_detail_hosts;
    int num_detail_host_ser = num_detail_hosts / 2;
    int num_detail_host_cli = num_detail_hosts / 2;
    
    // received_bytes.resize(num_dum_hosts, 0);
    
    NS_LOG_INFO("kvalue: " << k_value);
    NS_LOG_INFO("detail_host_percent: " << detail_host_percent);
    NS_LOG_INFO("total_hosts: " << total_hosts << " detail_host_num: " << num_detail_hosts);
    
    // Normal Fat Tree topology
    int num_spine_sw = (k_value / 2) * (k_value / 2);
    int num_pod = k_value;
    int num_agg_sw = k_value / 2;
    int racks_per_pod = k_value / 2; // num_tor_sw
    int num_hosts_per_rack = k_value / 2;
    
    // Calculate how many detailed hosts are in each rack
    int total_racks = num_pod * racks_per_pod;
    std::vector<DetailHostPair> detailHostPairs(total_racks, {0, 0});
    
    for (int i = 0; i < num_detail_host_ser; i++)
    {
        detailHostPairs[i % total_racks].num_detail_ser++;
        int cli_idx = total_racks - 1 - (i % total_racks);
        detailHostPairs[cli_idx].num_detail_cli++;
    }
    for (int i = 0; i < total_racks; i++)
    {
        NS_LOG_INFO("Rack " << i << " has " << detailHostPairs[i].num_detail_ser << " detail servers and "
                            << detailHostPairs[i].num_detail_cli << " detail clients");
    }


    // int num_spine_sw = n_spine_sw;
    // int num_pod = n_agg_bl;
    // int num_agg_sw = n_agg_sw;
    // int racks_per_pod = n_agg_racks;
    // int num_hosts_per_rack = h_per_rack;

    NodeContainer spine;
    NodeContainer pod_sw[num_pod][2]; // aggregation switches and tor

    // Nodes connected by core to aggregation switch links
    NodeContainer spine_agg[num_spine_sw][num_pod]; 
    // Nodes connected by aggregation swithces internal links
    NodeContainer agg_tor[num_pod][num_agg_sw][racks_per_pod]; 
    // Nodes connected by tor to host links
    NodeContainer tor_host[num_pod][racks_per_pod][num_hosts_per_rack];
    
    
    NetDeviceContainer spine_aggd[num_spine_sw][num_pod];
    NetDeviceContainer agg_tord[num_pod][num_agg_sw][racks_per_pod];
    NetDeviceContainer tor_hostd[num_pod][racks_per_pod][num_hosts_per_rack];
    NetDeviceContainer tord[num_pod][racks_per_pod];

    NetDeviceContainer agg_down_port[num_pod][racks_per_pod];
    //dummy host netdev of tor links, those assigned IP addresses
    NetDeviceContainer tor_dummy_host_port[num_pod][racks_per_pod]; 

    Ipv4InterfaceContainer agg_tor_ip[num_pod][racks_per_pod];
    Ipv4InterfaceContainer tor_dummy_host_ip[num_pod][racks_per_pod];
    Ipv4InterfaceContainer spine_agg_ip[num_spine_sw][num_pod];

    SimpleNetDeviceHelper simp_netdev;
    simp_netdev.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("5MB")));
    simp_netdev.SetDeviceAttribute("DataRate", DataRateValue(spine_agg_linkRate));
    simp_netdev.SetChannelAttribute("Delay", TimeValue(linkLatency));


    // Create Spine Nodes
    spine.Create(num_spine_sw);
    
    // Create Aggregation Switches and ToR Switches
    for (int i = 0; i < num_pod; i++){
        pod_sw[i][0].Create(num_agg_sw);
        pod_sw[i][1].Create(racks_per_pod);
    }

    // Connect Spine to Aggregation Switches
    for (int i = 0; i < num_spine_sw; i++){
        for (int j = 0; j < num_pod; j++){
            int agg_idx = i / num_agg_sw;
            // NS_LOG_INFO("spine " << i << " connected to aggblock " << j << " agg_idx " << agg_idx);
            spine_agg[i][j].Add(spine.Get(i));
            spine_agg[i][j].Add(pod_sw[j][0].Get(agg_idx));
            spine_aggd[i][j] = simp_netdev.Install(spine_agg[i][j]);
        }
    }
    
    simp_netdev.SetDeviceAttribute("DataRate", DataRateValue(agg_tor_linkRate));
    // Connect Aggregation Switches to ToR Switches
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < num_agg_sw; j++){
            for (int k = 0; k < racks_per_pod; k++){
                agg_tor[i][j][k].Add(pod_sw[i][0].Get(j));
                agg_tor[i][j][k].Add(pod_sw[i][1].Get(k));
                agg_tord[i][j][k] = simp_netdev.Install(agg_tor[i][j][k]);
                tord[i][k].Add(agg_tord[i][j][k].Get(1));
                agg_down_port[i][k].Add(agg_tord[i][j][k].Get(0));
            }
        }
    }
    simp_netdev.SetDeviceAttribute("DataRate", DataRateValue(linkRate));

    // Create Hosts and connect to ToR Switches
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            int num_dum_hosts = num_hosts_per_rack -  detailHostPairs[i * racks_per_pod + j].num_detail_ser - detailHostPairs[i * racks_per_pod + j].num_detail_cli;
            NS_LOG_INFO("Rack " << i * racks_per_pod + j << " has " << num_dum_hosts << " dummy hosts");

            for (int k = 0; k < num_dum_hosts; k++){
                tor_host[i][j][k].Add(pod_sw[i][1].Get(j)); // ToR switch node
                // Dummy host
                tor_host[i][j][k].Create(1); // Host node
                tor_hostd[i][j][k] = simp_netdev.Install(tor_host[i][j][k]);
                tord[i][j].Add(tor_hostd[i][j][k].Get(0));
                tor_dummy_host_port[i][j].Add(tor_hostd[i][j][k].Get(1));
            }
        }
    }

    // Create Bridge Net devices
    BridgeHelper bridge;
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            bridge.Install(pod_sw[i][1].Get(j), tord[i][j]);
        }
    }

    /************************************************************************/
    // Done with topology creation. Now set the software stack and App
    
    // Install IP stack to all nodes except Tor switches
    InternetStackHelper ip_stack;
    ip_stack.Install(spine);
    for (int i = 0; i < num_pod; i++){
        ip_stack.Install(pod_sw[i][0]);
    }
    

    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            int num_dum_hosts = num_hosts_per_rack -  detailHostPairs[i * racks_per_pod + j].num_detail_ser - detailHostPairs[i * racks_per_pod + j].num_detail_cli;
            // NS_LOG_INFO("Rack " << i * racks_per_pod + j << " has " << num_dum_hosts << " dummy hosts");

            for (int k = 0; k < num_dum_hosts; k++){
                ip_stack.Install(tor_host[i][j][k].Get(1));
            }
        }
    }

	Ipv4AddressHelper ipv4;
	int sub = 0;
	char* ip_base = new char[10];
    inc_address_base(ip_base, sub);
    
    // Assign IP addresses to all hosts
    // Each ToR switch has a /24 subnet starting from 10.0.0.0
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            NS_LOG_INFO(i << "th pod " << j << "th rack IP base: " << ip_base);
            int rack_idx = i * racks_per_pod + j;
            int dum_start_idx = detailHostPairs[rack_idx].num_detail_ser;
            ipv4.SetBase(ip_base, "255.255.255.0");
            // Install on agg_down_port first
            agg_tor_ip[i][j] = ipv4.Assign(agg_down_port[i][j]);
            // Install on dummy host
            std::ostringstream s_stream;
            s_stream << "0.0.0." << k_value/2 + 1 + dum_start_idx;
            std::string s = s_stream.str();
            ipv4.SetBase(ip_base, "255.255.255.0", s.c_str());
            tor_dummy_host_ip[i][j] = ipv4.Assign(tor_dummy_host_port[i][j]);
            inc_address_base(ip_base, sub);
        }
    }

    // Assign IP addresses to all spine-agg links
    // NS_LOG_INFO("IP base for spine-agg devs: " << ip_base);
    for (int i = 0; i < num_spine_sw; i++){
        for (int j = 0; j < num_pod; j++){
            ipv4.SetBase(ip_base, "255.255.255.0");
            spine_agg_ip[i][j] = ipv4.Assign(spine_aggd[i][j]);
            inc_address_base(ip_base, sub);
        }
    }
    

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    // Add SimbricksNetDevice to the bridge
    // Only after populating the routing tables
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){

            Ptr<Node> node = pod_sw[i][1].Get(j);
            Ptr<BridgeNetDevice> bridge;
            if (node) {
                NS_LOG_INFO("Node ID: " << node->GetId());
                for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
                    // NS_LOG_INFO("Device " << i << ": " << node->GetDevice(i)->GetInstanceTypeId());
                    if (node->GetDevice(i)->IsBridge()) {
                        NS_LOG_INFO("Device " << i << " is a BridgeNetDevice");
                        bridge = DynamicCast<BridgeNetDevice>(node->GetDevice(i));

                    }
                    else{
                        // NS_LOG_INFO("MAC addr of " << i << " th device: "<< node->GetDevice(i)->GetAddress() );
                    }
                }
            }

            if (bridge)
            {
                NS_LOG_INFO("Device Type: " << bridge->GetInstanceTypeId());
            }
            else{
                NS_LOG_WARN("No device found at the specified index.");

            }


            for (int k = 0; k < num_hosts_per_rack; k++){

                int host_idx = num_pod * racks_per_pod * k + i * racks_per_pod + j;
                // NS_LOG_INFO("Host " << host_idx << " connected to pod " << i << " rack " << j << " host " << k);
                if ((host_idx < num_detail_host_ser) || (host_idx > num_detail_host_ser + num_dum_hosts - 1)){
                    NS_LOG_INFO("Add detail host " << host_idx);
                    // Detailed host
                    int detail_host_idx;
                    if (host_idx < total_hosts / 2){
                        //server
                        detail_host_idx = host_idx * 2;
                    }
                    else{
                        //client
                        detail_host_idx = (total_hosts - 1 - host_idx) * 2 + 1;
                    }

                    std::ostringstream mac_stream;
                    mac_stream << "00:90:00:00:00:" << std::setw(2) << std::setfill('0') << std::hex << host_idx;
                    std::string mac_addr = mac_stream.str();
                    Mac48Address mac(mac_addr.c_str());
                    Ptr<simbricks::SimbricksNetDevice> device = CreateObject<simbricks::SimbricksNetDevice> ();
                    if (!device)
                    {
                        NS_LOG_INFO("Failed to create SimbricksNetDevice");
                        return 1;
                    }
                    std::string& cpp = simbricksPortPaths[detail_host_idx];
                    device->SetAttribute("UnixSocket", StringValue(cpp));
                    device->SetAddress(mac);
                    NS_LOG_INFO("MAC = " << device->GetAddress() );
                    node->AddDevice(device);
                    bridge->AddBridgePort(device);
                    device->Start();
                    tord[i][j].Add(device);
                }
            }
        }
    }
    // simp_netdev.EnablePcapAll("fat_tree");
    

    // Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(0.1),
    //     Create<OutputStreamWrapper>("dynamic-global-routing.routes", std::ios::out), Time::S);

    // Install sink and client applications
    // in tord_op_ip[i][j], the host IPs start from GetAddress(num_agg_sw)
    // in tor_host[i][j][k], the host Node is Get(1) 

    // NS_LOG_INFO("Sink info");
    // NS_LOG_INFO(tord_op_ip[0][0].GetAddress(2));
    // NS_LOG_INFO(tor_host[0][0][0].Get(1)->GetId());

    // sink(tord_op_ip[0][0].GetAddress(2), tor_host[0][0][0].Get(1));
    // client(tord_op_ip[0][0].GetAddress(2), tor_host[0][0][1].Get(1), flow_size);
    // client(tord_op_ip[0][0].GetAddress(2), tor_host[0][1][0].Get(1), flow_size);
    // client(tord_op_ip[0][0].GetAddress(2), tor_host[3][0][0].Get(1), flow_size);
    
    
    // different pod
    // sink(tor_dummy_host_ip[0][0].GetAddress(0), tor_host[0][0][0].Get(1));
    // client(tor_dummy_host_ip[0][0].GetAddress(0), tor_host[3][1][0].Get(1), flow_size);
    // different rack in the same pod
    sink(tor_dummy_host_ip[1][0].GetAddress(2), tor_host[1][0][2].Get(1));
    client(tor_dummy_host_ip[1][0].GetAddress(2), tor_host[1][1][0].Get(1), flow_size);
    // same rack
    // sink(tor_dummy_host_ip[0][0].GetAddress(1), tor_host[0][0][1].Get(1));
    // client(tor_dummy_host_ip[0][0].GetAddress(1), tor_host[0][0][3].Get(1), flow_size);

/*
    
    int host_ip_start = k_value / 2; 
    for (int i = 0; i < num_pod; i++){
        for (int j = 0; j < racks_per_pod; j++){
            int dum_start_idx = detailHostPairs[i * racks_per_pod + j].num_detail_ser;


            for (int k = 0; k < num_hosts_per_rack; k++){
                int host_idx = num_pod * racks_per_pod * k + i * racks_per_pod + j;
                if (host_idx >= num_detail_host_ser && host_idx < total_hosts/2 ){
                    // NS_LOG_INFO("size of tor_dummy_host_ip["<< i << "][" <<j <<"] " << tor_dummy_host_ip[i][j].GetN() );
                    // Server
                    sink(tor_dummy_host_ip[i][j].GetAddress(k), tor_host[i][j][k-dum_start_idx].Get(1));
                    NS_LOG_INFO("Install Sink on host ID " << host_idx << "; " << " N th tor dev: " << k - dum_start_idx );
                    // Client
                    int client_idx = total_hosts - host_idx - 1;
                    struct HostPos client_pos = GetPodRackHostFromHostIdx(client_idx, num_pod, racks_per_pod, num_hosts_per_rack);
                    int dum_start_idx_cli = detailHostPairs[client_pos.pod * racks_per_pod + client_pos.rack].num_detail_ser;
                    // NS_LOG_INFO("Install Client on host ID " << client_idx  << "; " << " N th tor dev: "<< client_pos.host - dum_start_idx_cli);
                    NS_LOG_INFO("Client pod: " << client_pos.pod << " rack:" << client_pos.rack << " host: " << client_pos.host << " dum_start_idx: " << dum_start_idx_cli);
                    client(tor_dummy_host_ip[i][j].GetAddress(k), tor_host[client_pos.pod][client_pos.rack][client_pos.host - dum_start_idx_cli].Get(1), flow_size);

                }

                }
            }
    }
*/


    // Print all NetDevices and their IP addresses
    // PrintAllNetDeviceIPs();

    // Ptr<FlowMonitor> flowMonitor;
    // FlowMonitorHelper flowHelper;
    // flowMonitor = flowHelper.InstallAll();

    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
    Simulator::Schedule(Seconds(0.0), &PrintSimProgress);
    Simulator::Schedule(Seconds(START + HEAD_ROOM), &PrintClientStart);
    Simulator::Stop(Seconds(END));

    NS_LOG_INFO("Run.");
    Simulator::Run();

    // flowMonitor->SerializeToXmlFile("flowmon-results.xml", true, true);
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}
