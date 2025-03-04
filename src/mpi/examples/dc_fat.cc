#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <chrono>

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
  
	CommandLine cmd(__FILE__);
	cmd.Parse(argc, argv);

	int k = 4;
	int core_k = (k/2)*(k/2);

	// LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

	NodeContainer core;
	NodeContainer agg[k][2];
	core.Create(core_k);


	NodeContainer coreagg[core_k][k];
	NodeContainer aggint[k][k/2][k/2];
	NodeContainer edge[k][k/2][k/2];

	NetDeviceContainer coreaggd[core_k][k];
	NetDeviceContainer aggintd[k][k/2][k/2];
	NetDeviceContainer edged[k][k/2][k/2];
	
	Ipv4InterfaceContainer coreaggi[core_k][k];
	Ipv4InterfaceContainer agginti[k][k/2][k/2];
	Ipv4InterfaceContainer edgei[k][k/2][k/2];

	PointToPointHelper ptp1;
	ptp1.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
	ptp1.SetChannelAttribute ("Delay", StringValue ("500ns"));

	InternetStackHelper stack;

	Ipv4AddressHelper address;

	int sub = 0;
	char* char_array = new char[10];

    inc_address_base(char_array, sub);
	address.SetBase (char_array, "255.255.255.0");

	for(int i=0;i<k;i++){
		agg[i][0].Create(k/2);
		agg[i][1].Create(k/2);
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
				aggintd[i][j][l] = ptp1.Install (aggint[i][j][l]);
			}
		}
	}

	for(int i=0;i<k;i++){
		for(int j=0;j<k/2;j++){
			for(int l=0;l<k/2;l++){
				edge[i][j][l].Add(agg[i][1].Get(j));
				edge[i][j][l].Create(1);
				edged[i][j][l] = ptp1.Install (edge[i][j][l]);
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
		if(1){
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
	
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::milliseconds;
	auto t1 = high_resolution_clock::now();

	Simulator::Run ();

	auto t2 = high_resolution_clock::now();
	duration<double, std::milli> ms_double = t2 - t1;
	std::cout << "Runtime = " << ms_double.count()/1000 << std::endl;

	Simulator::Destroy ();
	return 0;
} 
