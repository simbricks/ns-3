/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017-20 NITK Surathkal
 * Copyright (c) 2020 Tom Henderson (better alignment with experiment)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Shravya K.S. <shravya.ks0@gmail.com>
 *          Apoorva Bhargava <apoorvabhargava13@gmail.com>
 *          Shikha Bakshi <shikhabakshi912@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Tom Henderson <tomh@tomh.org>
 */


// The topology is roughly as follows
//
//  R1         S1
//  |   (10G)   | 
//  T1 ------- T2
//              
//           
//
// The link between bridge T1 and T2 is 10 Gbps.  All other
// links are unlimitted
//
//
//
// RED queues configured for ECN marking are used at the bottlenecks.
// This program runs the program by default for 10 seconds.
//
// The program outputs six files.  The first three:
// * dctcp-modes-throughput.dat
// * dctcp-example-s2-r2-throughput.dat
// * dctcp-example-s3-r1-throughput.dat
// * dctcp-example-t1-length.dat
// report on the bottleneck queue length (in packets and microseconds
// of delay) at 10 ms intervals during the measurement window.
//
//
// The RED parameters (min_th and max_th) are set to the same values as
// reported in the paper, but we observed that throughput distributions
// and queue delays are very sensitive to these parameters, as was also
// observed in the paper; it is likely that the paper's throughput results
// could be achieved by further tuning of the RED parameters.  However,
// the default results show that DCTCP is able to achieve high link
// utilization and low queueing delay and fairness across competing flows
// sharing the same path.

#include <iostream>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

std::stringstream filePlotQueue1;
std::ofstream rxS1R1Throughput;
std::ofstream t1QueueLength;

std::vector<uint64_t> rxS1R1Bytes;


void
PrintProgress (Time interval)
{
  std::cout << "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << " seconds simulation time" << std::endl;
  Simulator::Schedule (interval, &PrintProgress, interval);
}

void
TraceS1R1Sink (std::size_t index, Ptr<const Packet> p, const Address& a)
{
  rxS1R1Bytes[index] += p->GetSize ();
}


void
InitializeCounters (size_t num_pairs)
{
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      rxS1R1Bytes[i] = 0;
    }

}

void
PrintThroughput (Time measurementWindow, size_t num_pairs)
{
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      rxS1R1Throughput << measurementWindow.GetSeconds () << "s " << i << " " << (rxS1R1Bytes[i] * 8) / (measurementWindow.GetSeconds ()) / 1e6 << std::endl;
    }

}


void
CheckT1QueueSize (Ptr<QueueDisc> queue)
{
  // 1500 byte packets
  uint32_t qSize = queue->GetNPackets ();
  Time backlog = Seconds (static_cast<double> (qSize * 1500 * 8) / 1e10); // 10 Gb/s
  // report size in units of packets and ms
  t1QueueLength << std::fixed << std::setprecision (2) << Simulator::Now ().GetSeconds () << " " << qSize << " " << backlog.GetMicroSeconds () << std::endl;
  // check queue size every 1/100 of a second
  Simulator::Schedule (MilliSeconds (10), &CheckT1QueueSize, queue);
}

int main (int argc, char *argv[])
{
  std::string outputFilePath = ".";
  std::string tcpTypeId = "TcpDctcp";
  Time flowStartupWindow = Seconds (0);
  Time convergenceTime = Seconds (0);
  Time measurementWindow = Seconds (1);
  bool enableSwitchEcn = true;
  Time progressInterval = MilliSeconds (100);
  size_t num_pairs = 2;
  double EcnTh = 20000;
  uint32_t mtu = 1500;

  LogComponentEnable ("RedQueueDisc", LOG_LEVEL_ALL);
  LogComponentEnable ("QueueDisc", LOG_LEVEL_ALL);

  CommandLine cmd (__FILE__);
  cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
  cmd.AddValue ("flowStartupWindow", "startup time window (TCP staggered starts)", flowStartupWindow);
  cmd.AddValue ("convergenceTime", "convergence time", convergenceTime);
  cmd.AddValue ("measurementWindow", "measurement window", measurementWindow);
  cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
  cmd.AddValue ("EcnTh", "K value at switches", EcnTh);
  cmd.AddValue ("mtu", "Ethernet mtu", mtu);
  cmd.Parse (argc, argv);

  
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));

  Time startTime = Seconds (0);
  Time stopTime = flowStartupWindow + convergenceTime + measurementWindow;

  Time clientStartTime = startTime;

  rxS1R1Bytes.reserve (num_pairs);

  NodeContainer S1, R1;
  Ptr<Node> T1 = CreateObject<Node> ();
  Ptr<Node> T2 = CreateObject<Node> ();
  R1.Create (num_pairs);
  S1.Create (num_pairs);


  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (mtu-52));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // Set default parameters for RED queue disc
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
  // ARED may be used but the queueing delays will increase; it is disabled
  // here because the SIGCOMM paper did not mention it
  // Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
  // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (mtu));
  // Triumph and Scorpion switches used in DCTCP Paper have 4 MB of buffer
  // If every packet is 1500 bytes, 2666 packets can be stored in 4 MB
  //Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("2666p")));
  //Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("4MB")));
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, 4000000)));
  // DCTCP tracks instantaneous queue length only; so set QW = 1
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (20));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (20));

  PointToPointHelper pointToPointSR;
  pointToPointSR.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPointSR.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  pointToPointSR.SetChannelAttribute ("Delay", StringValue ("500ns"));
  
  PointToPointHelper pointToPointT;
  pointToPointT.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPointSR.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  pointToPointT.SetChannelAttribute ("Delay", StringValue ("500ns"));


  // Create a total of 5 links.
  std::vector<NetDeviceContainer> S1T2;
  S1T2.reserve (num_pairs);
  std::vector<NetDeviceContainer> R1T1;
  R1T1.reserve (num_pairs);
  NetDeviceContainer T1T2 = pointToPointT.Install (T1, T2);

  for (std::size_t i = 0; i < num_pairs; i++)
    {
      Ptr<Node> n = S1.Get (i);
      S1T2.push_back (pointToPointSR.Install (n, T2));
    }
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      Ptr<Node> n = R1.Get (i);
      R1T1.push_back (pointToPointSR.Install (n, T1));
    }

  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tchRed10;
  // MinTh = 50, MaxTh = 150 recommended in ACM SIGCOMM 2010 DCTCP Paper
  // This yields a target (MinTh) queue depth of 60us at 10 Gb/s
  tchRed10.SetRootQueueDisc ("ns3::RedQueueDisc",
                             "LinkBandwidth", StringValue ("10Gbps"),
                             "LinkDelay", StringValue ("500ns"),
                             "MinTh", DoubleValue (EcnTh),
                             "MaxTh", DoubleValue (EcnTh));
  QueueDiscContainer queueDiscs1 = tchRed10.Install (T1T2);



  Ipv4AddressHelper address;
  std::vector<Ipv4InterfaceContainer> ipS1T2;
  ipS1T2.reserve (num_pairs);
  std::vector<Ipv4InterfaceContainer> ipR1T1;
  ipR1T1.reserve (num_pairs);

  address.SetBase ("192.168.64.0", "255.255.255.0");
  Ipv4InterfaceContainer ipT1T2 = address.Assign (T1T2);

  address.SetBase ("10.1.1.0", "255.255.255.0");
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      ipR1T1.push_back (address.Assign (R1T1[i]));
      address.NewNetwork ();
    }

  address.SetBase ("10.2.1.0", "255.255.255.0");
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      ipS1T2.push_back (address.Assign (S1T2[i]));
      address.NewNetwork ();
    }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Each sender in S1 sends to a receiver in R1
  std::vector<Ptr<PacketSink> > r1Sinks;
  r1Sinks.reserve (num_pairs);
  for (std::size_t i = 0; i < num_pairs; i++)
    {
      uint16_t port = 50000 + i;
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      ApplicationContainer sinkApp = sinkHelper.Install (R1.Get (i));
      Ptr<PacketSink> packetSink = sinkApp.Get (0)->GetObject<PacketSink> ();
      r1Sinks.push_back (packetSink);
      sinkApp.Start (startTime);
      sinkApp.Stop (stopTime);

      OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
      clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Gbps")));
      clientHelper1.SetAttribute ("PacketSize", UintegerValue (mtu - 52));

      ApplicationContainer clientApps1;
      AddressValue remoteAddress (InetSocketAddress (ipR1T1[i].GetAddress (0), port));
      clientHelper1.SetAttribute ("Remote", remoteAddress);
      clientApps1.Add (clientHelper1.Install (S1.Get (i)));
      clientApps1.Start (clientStartTime);
      clientApps1.Stop (stopTime);
    }

  std::ostringstream tput;
  std::ostringstream qlen;
  tput << "dctcp-modes-tput-" << mtu << "-" << EcnTh << ".dat";
  qlen << "dctcp-modes-qlen-" << mtu << "-" << EcnTh << ".dat";
  std::string f_tput = tput.str ();
  std::string f_qlen = qlen.str ();

  rxS1R1Throughput.open (f_tput, std::ios::out);
  rxS1R1Throughput << "#Time(s) flow thruput(Mb/s) MTU: " << mtu << " K_value: " << EcnTh << std::endl;
  t1QueueLength.open (f_qlen, std::ios::out);
  t1QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;

  for (std::size_t i = 0; i < num_pairs; i++)
    {
      r1Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS1R1Sink, i));
    }

  Simulator::Schedule (flowStartupWindow + convergenceTime, &InitializeCounters, num_pairs);
  Simulator::Schedule (flowStartupWindow + convergenceTime + measurementWindow, &PrintThroughput, measurementWindow, num_pairs);
  Simulator::Schedule (progressInterval, &PrintProgress, progressInterval);
  Simulator::Schedule (flowStartupWindow + convergenceTime, &CheckT1QueueSize, queueDiscs1.Get (1));
  Simulator::Stop (stopTime + TimeStep (1));

  //pointToPointT.EnablePcapAll ("modes-pcap");

  Simulator::Run ();

  rxS1R1Throughput.close ();
  t1QueueLength.close ();
  Simulator::Destroy ();
  return 0;
}
