#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mpi-interface.h"
#include <iomanip>
#include <mpi.h>

#define START 0.0
#define END 0.01

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DataCenter");

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

void sink(ns3::Ipv4Address add, ns3::Ptr<Node> node){
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",InetSocketAddress(add,8080));
	ApplicationContainer sinkApp = packetSinkHelper.Install(node);
	sinkApp.Start(Seconds(START));
	sinkApp.Stop(Seconds(END));
}

void client(ns3::Ipv4Address add, ns3::Ptr<Node> node){
	OnOffHelper client("ns3::TcpSocketFactory", InetSocketAddress(add, 8080));
	client.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000000000000]"));
	client.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	client.SetAttribute ("DataRate", DataRateValue (DataRate ("1000Mbps")));
	client.SetAttribute ("PacketSize", UintegerValue (200));
	
	ApplicationContainer clientApp = client.Install (node);
	clientApp.Start(Seconds (START));
	clientApp.Stop (Seconds (END));
}

int main (int argc, char *argv[])
{
	LogComponentEnable("PacketSink",(LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
	LogComponentEnable("DataCenter",(LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));

	bool nix = true;
    bool nullmsg = false;
    bool tracing = false;
    bool testing = false;
    bool verbose = false;

    // Parse command line
    CommandLine cmd(__FILE__);
    cmd.AddValue("nix", "Enable the use of nix-vector or global routing", nix);
    cmd.AddValue("nullmsg", "Enable the use of null-message synchronization", nullmsg);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.AddValue("verbose", "verbose output", verbose);
    cmd.AddValue("test", "Enable regression test output", testing);
    cmd.Parse(argc, argv);

    // Distributed simulation setup; by default use granted time window algorithm.
    if (nullmsg)
    {
        GlobalValue::Bind("SimulatorImplementationType",
                          StringValue("ns3::NullMessageSimulatorImpl"));
    }
    else
    {
        GlobalValue::Bind("SimulatorImplementationType",
                          StringValue("ns3::DistributedSimulatorImpl"));
    }

    // Enable parallel simulator with the command line arguments
    MpiInterface::Enable(&argc, &argv);

    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();

	int k = 4;
	int core_k = (k/2)*(k/2);

    if (systemCount != k+1)
    {
        NS_LOG_INFO("Check number of logical processors.");
        return 1;
    }


	NodeContainer core;
	NodeContainer agg[k][2];
	core.Create(core_k,k);


	NodeContainer coreagg[core_k][k];
	NodeContainer aggint[k][k/2][k/2];
	NodeContainer edge[k][k/2][k/2];

	NetDeviceContainer coreaggd[core_k][k];
	NetDeviceContainer aggintd[k][k/2][k/2];
	NetDeviceContainer edged[k][k/2][k/2];
	
	Ipv4InterfaceContainer coreaggi[core_k][k];
	Ipv4InterfaceContainer agginti[k][k/2][k/2];
	Ipv4InterfaceContainer edgei[k][k/2][k/2];

	PointToPointHelper ptp1,ptp2,ptp3;
	ptp1.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
	ptp1.SetChannelAttribute ("Delay", StringValue ("500ns"));
	ptp2.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
	ptp2.SetChannelAttribute ("Delay", StringValue ("500ns"));
	ptp3.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
	ptp3.SetChannelAttribute ("Delay", StringValue ("500ns"));

	InternetStackHelper stack;

	Ipv4AddressHelper address;

	int sub = 0;
	char* char_array = new char[10];

    inc_address_base(char_array, sub);
	address.SetBase (char_array, "255.255.255.0");

	for(int i=0;i<k;i++){
		agg[i][0].Create(k/2,i);
		agg[i][1].Create(k/2,i);
	}

	for(int i=0;i<core_k;i++){
		for(int j=0;j<k;j++){
			coreagg[i][j].Add(core.Get(i));
			coreagg[i][j].Add(agg[j][0].Get(i/(k/2)));
			coreaggd[i][j] = ptp1.Install (coreagg[i][j]);
		}
	}

	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				aggint[i][j][l].Add(agg[i][0].Get(j));
				aggint[i][j][l].Add(agg[i][1].Get(l));
				aggintd[i][j][l] = ptp2.Install (aggint[i][j][l]);
			}
		}
	}

	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				edge[i][j][l].Add(agg[i][1].Get(j));
				edge[i][j][l].Create(1,i);
				edged[i][j][l] = ptp3.Install (edge[i][j][l]);
			}
		}
	}

	stack.Install(core);
	for(int i=0;i<k;i++){
		stack.Install(agg[i][0]);
		stack.Install(agg[i][1]);
	}
	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				stack.Install(edge[i][j][l].Get(1));
			}
		}
	}

	for(int i=0;i<core_k;i++){
		for(int j=0;j<k;j++){
			coreaggi[i][j] = address.Assign(coreaggd[i][j]);
			inc_address_base(char_array, sub);
			address.SetBase (char_array, "255.255.255.0");
		}
	}

	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				agginti[i][j][l] = address.Assign(aggintd[i][j][l]);
				inc_address_base(char_array, sub);
				address.SetBase (char_array, "255.255.255.0");
			}
		}
	}

	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				edgei[i][j][l] = address.Assign(edged[i][j][l]);
				inc_address_base(char_array, sub);
				address.SetBase (char_array, "255.255.255.0");
			}
		}
	}

	for(int i=0;i<4;i++){
		// if(!systemId) LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
		if(systemId==i){
			sink(edgei[i][0][0].GetAddress(1), edge[i][0][0].Get(1));
			client(edgei[i][0][0].GetAddress(1),edge[i][1][1].Get(1));
			client(edgei[i][0][1].GetAddress(1),edge[i][1][1].Get(1));
			client(edgei[i][0][0].GetAddress(1),edge[i][1][0].Get(1));
			sink(edgei[i][0][1].GetAddress(1), edge[i][0][1].Get(1));
			client(edgei[(i+1)%k][0][1].GetAddress(1),edge[i][1][0].Get(1));	
			client(edgei[(i+2)%k][0][1].GetAddress(1),edge[i][0][0].Get(1));
			client(edgei[(i+3)%k][0][1].GetAddress(1),edge[i][0][1].Get(1));
		}
	}

	// Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	Simulator::Stop(Seconds(END));

	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	Simulator::Run ();

	MPI_Barrier(MPI_COMM_WORLD);
	double end = MPI_Wtime();
	Simulator::Destroy ();
	MpiInterface::Disable();
	if (systemId == 0) { /* use time on master node */
		NS_LOG_INFO("Runtime = " << end-start);
	}
	return 0;
} 
