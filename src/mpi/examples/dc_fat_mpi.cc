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

/*
FatTree topology decribed in paper "A Scalable, Commodity Data Center Network Architecture, Mohammad AI-Fares, Alexander Loukissas, Amin Vadat; SIGCOMM'08"

Example command to execute this script
./ns3 run dc_fat_mpi --command-template="/usr/bin/mpiexec --allow-run-as-root -np 5 %s --num_pod=4"
*/


#define START 0.0
#define END 0.1
#define NUM_STEPS 20

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
	client.SetAttribute ("PacketSize", UintegerValue (400));
	
	ApplicationContainer clientApp = client.Install (node);
	clientApp.Start(Seconds (START));
	clientApp.Stop (Seconds (END));
}

void PrintSimProgress(){
	float step = (END - START) / NUM_STEPS;

	NS_LOG_INFO("Sim. Time: " << Simulator::Now().GetMilliSeconds() << " ms");
	Simulator::Schedule(Seconds(step), &PrintSimProgress);
}

void PrintSinkRx(int pod, int agg, int host, ns3::Ptr<PacketSink> sink){
	uint64_t totalRx = sink->GetTotalRx();
	NS_LOG_INFO("Pod: " << pod << " Agg: " << agg << " Host: " << host << " TotalRx: " << totalRx);
}

int main (int argc, char *argv[])
{
	// LogComponentEnable("PacketSink",(LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));
	LogComponentEnable("DataCenter",(LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));

	bool nix = true;
    bool nullmsg = false;
    bool tracing = false;
    bool testing = false;
    bool verbose = false;
	int num_pod = 4;
	bool internal_traffic = true;

    // Parse command line
    CommandLine cmd(__FILE__);
	cmd.AddValue("num_pod", "number of pod in FatTree topoplogy", num_pod);
    cmd.AddValue("nix", "Enable the use of nix-vector or global routing", nix);
    cmd.AddValue("nullmsg", "Enable the use of null-message synchronization", nullmsg);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.AddValue("verbose", "verbose output", verbose);
    cmd.AddValue("test", "Enable regression test output", testing);
	cmd.AddValue("internal_traffic", "Traffic consumed in the rack", internal_traffic);
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

	int k = num_pod; // number of pods
	int core_k = (k/2)*(k/2); //number of core switches
	
	int total_racks = k * ( k / 2 );
	int racks_per_pod = k / 2;
	int per_lp_racks = total_racks / systemCount;
	int rack_idx_start = 0;
	

	// Create core switch nodes in systemId == 0
	NodeContainer core;
	NodeContainer agg[k][2];
	core.Create(core_k,0);
	

	NodeContainer coreagg[core_k][k]; // core to aggregation switch link nodes
	NodeContainer aggint[k][k/2][k/2]; // aggregation swithces internal link nodes
	NodeContainer edge[k][k/2][k/2]; // edge to host link nodes

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

	// Create aggregation switch nodes
	for(int i=0;i<k;i++){
		agg[i][0].Create(k/2,0);
	}
	
	for (int i = 0; i < k; i++){
		for (int j = 0; j < racks_per_pod; j++){
			int lp_idx = i * racks_per_pod + j;
			int sys_id = lp_idx / per_lp_racks;
			if (systemId == 0)
				NS_LOG_INFO("agg: " << lp_idx  << " SysId: " << sys_id);
			Ptr<Node> node = CreateObject<Node>(sys_id);

			agg[i][1].Add(node);
		}

	}

	// Create ptp links between core and aggregation switches
	for(int i=0;i<core_k;i++){
		for(int j=0;j<k;j++){
			coreagg[i][j].Add(core.Get(i));
			coreagg[i][j].Add(agg[j][0].Get(i/(k/2)));
			coreaggd[i][j] = ptp1.Install (coreagg[i][j]);
		}
	}
	
	// Create ptp links between aggregation and edge switches
	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				aggint[i][j][l].Add(agg[i][0].Get(j));
				aggint[i][j][l].Add(agg[i][1].Get(l));
				aggintd[i][j][l] = ptp2.Install (aggint[i][j][l]);
			}
		}
	}

	// Create end-host nodes
	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			int lp_idx = i * racks_per_pod + j;
			int sys_id = lp_idx / per_lp_racks;
			for(int l=0;l<k/2;l++){
				edge[i][j][l].Create(1,sys_id);
			}
		}
	}

	// Create ptp links between edge swithces and end-hosts
	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				edge[i][j][l].Add(agg[i][1].Get(j));
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
				stack.Install(edge[i][j][l].Get(0));
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

	
	for(uint32_t i = 0; i < systemCount; i++){
		int rack_idx_end = rack_idx_start + per_lp_racks;
		
		// if(!systemId) LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
		if(systemId == i){
			NS_LOG_INFO("SystemId: " << systemId << " RackIdxStart: " << rack_idx_start << " RackIdxEnd: " << rack_idx_end);
			for (int r = rack_idx_start; r < rack_idx_end; r++){
				int pod_idx =  r / racks_per_pod;
				int agg_idx = r % racks_per_pod;
				if (internal_traffic){
					// the first host in rack as the sink and the rest of it as clients 
					sink(edgei[pod_idx][agg_idx][0].GetAddress(0), edge[pod_idx][agg_idx][0].Get(0));
					Simulator::Schedule(Seconds(END), &PrintSinkRx, pod_idx, agg_idx, 0, DynamicCast<PacketSink>(edge[pod_idx][agg_idx][0].Get(0)->GetApplication(0)));
					NS_LOG_INFO("Sink at: " << edgei[pod_idx][agg_idx][0].GetAddress(0));

					for (int c = 1; c < k/2; c++){
						client(edgei[pod_idx][agg_idx][0].GetAddress(0), edge[pod_idx][agg_idx][c].Get(0));
					}
				}
				else{
					if (agg_idx % 2){
						// an odd rack, all hosts are clients send to sinks in the next pod
						client(edgei[(pod_idx+1) % k][agg_idx - 1][0].GetAddress(0),edge[pod_idx][agg_idx][0].Get(0));
						client(edgei[(pod_idx+1) % k][agg_idx - 1][1].GetAddress(0),edge[pod_idx][agg_idx][1].Get(0));
					}
					else{
						sink(edgei[pod_idx][agg_idx][0].GetAddress(0), edge[pod_idx][agg_idx][0].Get(0));
						sink(edgei[pod_idx][agg_idx][1].GetAddress(0), edge[pod_idx][agg_idx][1].Get(0));
					}
				}
			}
		}
		rack_idx_start = rack_idx_end;
		
	}
	

	// Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
	if (systemId == 0){
		Simulator::Schedule(Seconds(0.0), &PrintSimProgress);
		// LogComponentEnable("PacketSink",(LogLevel)(LOG_LEVEL_INFO | LOG_PREFIX_NODE | LOG_PREFIX_TIME));

	}

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
