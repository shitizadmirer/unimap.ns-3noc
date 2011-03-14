/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 - 2011
 *               - Advanced Computer Architecture and Processing Systems (ACAPS),
 *               						Lucian Blaga University of Sibiu, Romania
 *               - Systems and Networking, University of Augsburg, Germany
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
 * Author: Ciprian Radu <ciprian.radu@ulbsibiu.ro>
 *         http://webspace.ulbsibiu.ro/ciprian.radu/
 */

// This example is based on the CSMA example csma-packet-socket.cc
//
// Network topology: 2D torus
//
//
//

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/topology-module.h"
#include "ns3/noc-sync-application.h"
#include "ns3/noc-sync-application-helper.h"
#include "ns3/noc-node.h"
#include "ns3/mobility-helper.h"
//#include "ns3/gtk-config-store.h"
#include "ns3/slb-load-router-component.h"
#include "ns3/so-load-router-component.h"
#include "ns3/noc-registry.h"
#include "ns3/integer.h"
#include "ns3/boolean.h"
#include "ns3/stats-module.h"
#include "ns3/noc-packet-tag.h"
#include "ns3/nstime.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/uinteger.h"
#include "src/noc/topology/noc-torus-2d.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NocSyncTest");

uint32_t numberOfNodes = 16;
uint32_t hSize = 4;

int
main (int argc, char *argv[])
{
  NS_ASSERT_MSG (numberOfNodes % hSize == 0,
      "The number of nodes ("<< numberOfNodes
      <<") must be a multiple of the number of nodes on the horizontal axis ("
      << hSize << ")");

  double injectionProbability (1);
  int dataPacketSpeedup (1);
  Time globalClock = PicoSeconds (1000); // 1 ns -> NoC @ 1GHz

  // Set up command line parameters used to control the experiment.
  CommandLine cmd;
  cmd.AddValue<double> ("injection-probability", "The packet injection probability.", injectionProbability);
  cmd.AddValue<int> ("data-packet-speedup", "The speedup used for data packets (compared to head packets)", dataPacketSpeedup);
  cmd.Parse (argc, argv);

  // set the global parameters
  NocRegistry::GetInstance ()->SetAttribute ("DataPacketSpeedup", IntegerValue (dataPacketSpeedup));
  NocRegistry::GetInstance ()->SetAttribute ("GlobalClock", TimeValue (globalClock));

  // Here, we will explicitly create four nodes.
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  for (unsigned int i = 0; i < numberOfNodes; ++i)
    {
      Ptr<NocNode> nocNode = CreateObject<NocNode> ();
      nodes.Add (nocNode);
    }
  //  nodes.Create (numberOfNodes);

  // use a helper function to connect our nodes to the shared channel.
  NS_LOG_INFO ("Build Topology.");
  Ptr<NocTopology> noc = CreateObject<NocMesh2D> ();
  // Ptr<NocTopology> noc = CreateObject<NocIrvineMesh2D> ();
  // Ptr<NocTopology> noc = CreateObject<NocTorus2D> ();
  noc->SetAttribute ("hSize", UintegerValue (hSize));

  uint32_t flitSize = 32; // 4 bytes
  NocRegistry::GetInstance ()->SetAttribute ("FlitSize", IntegerValue (flitSize));

  // set channel bandwidth to 1 flit / network clock
  // the channel's bandwidth is obviously expressed in bits / s
  // however, in order to avoid losing precision, we work with PicoSeconds (instead of Seconds)
  noc->SetChannelAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (1e12 * (flitSize)
      / globalClock.GetPicoSeconds ()))));
  // the channel has no propagation delay
  noc->SetChannelAttribute ("Delay", TimeValue (PicoSeconds (0)));

  //  noc->SetChannelAttribute ("FullDuplex", BooleanValue (false));
  //  noc->SetChannelAttribute ("Length", DoubleValue (10)); // 10 micro-meters channel length
  noc->SetInQueue ("ns3::DropTailQueue", "Mode", EnumValue (DropTailQueue::PACKETS), "MaxPackets", UintegerValue (1)); // the in queue must have at least 1 packet

  // install the topology

  noc->SetRouter ("ns3::FourWayRouter");
  //  noc->SetRouter ("ns3::FourWayLoadRouter");
  //  noc->SetRouter ("ns3::IrvineLoadRouter");
  //  noc->SetRouter ("ns3::IrvineRouter");

  // noc->SetRouterAttribute ("LoadComponent", TypeIdValue (TypeId::LookupByName ("ns3::SlbLoadRouterComponent")));
  // noc->SetRouterAttribute ("LoadComponent", TypeIdValue (TypeId::LookupByName ("ns3::SoLoadRouterComponent")));
  // Do not forget about changing the routing protocol when changing the load router component

   noc->SetRoutingProtocol ("ns3::XyRouting");
  // noc->SetRoutingProtocolAttribute ("RouteXFirst", BooleanValue (false));

  // noc->SetRoutingProtocol ("ns3::SlbRouting");
  // noc->SetRoutingProtocolAttribute ("LoadThreshold", IntegerValue (30));

  // noc->SetRoutingProtocol ("ns3::SoRouting");

  noc->SetSwitchingProtocol ("ns3::WormholeSwitching");
  // noc->SetSwitchingProtocol ("ns3::SafSwitching");
  // noc->SetSwitchingProtocol ("ns3::VctSwitching");

  NetDeviceContainer devs = noc->Install (nodes);
  NocRegistry::GetInstance ()->SetAttribute ("NoCTopology", PointerValue (noc));
  // done with installing the topology

  uint64_t packetLength = 5; // flits per packet

  NS_LOG_INFO ("Create Applications.");
  NocSyncApplicationHelper nocSyncAppHelper1 (nodes, devs, hSize);
  nocSyncAppHelper1.SetAttribute ("InjectionProbability", DoubleValue (injectionProbability));
  nocSyncAppHelper1.SetAttribute ("TrafficPattern", EnumValue (NocSyncApplication::DESTINATION_SPECIFIED));
  nocSyncAppHelper1.SetAttribute ("Destination", UintegerValue (10)); // destination
  nocSyncAppHelper1.SetAttribute ("MaxFlits", UintegerValue (packetLength));
  ApplicationContainer apps1 = nocSyncAppHelper1.Install (nodes.Get (2)); // source
  //  apps1.Start (Seconds (0.0));
  //  apps1.Stop (Seconds (10.0));

  NocSyncApplicationHelper nocSyncAppHelper2 (nodes, devs, hSize);
  nocSyncAppHelper2.SetAttribute ("InjectionProbability", DoubleValue (injectionProbability));
  nocSyncAppHelper2.SetAttribute ("TrafficPattern", EnumValue (NocSyncApplication::DESTINATION_SPECIFIED));
  nocSyncAppHelper2.SetAttribute ("Destination", UintegerValue (2)); // destination
  nocSyncAppHelper2.SetAttribute ("MaxFlits", UintegerValue (packetLength));
  ApplicationContainer apps2 = nocSyncAppHelper2.Install (nodes.Get (10)); // source
  //  apps2.Start (Seconds (0.0));
  //  apps2.Stop (Seconds (10.0));

  NocSyncApplicationHelper nocSyncAppHelper3 (nodes, devs, hSize);
  nocSyncAppHelper3.SetAttribute ("InjectionProbability", DoubleValue (injectionProbability));
  nocSyncAppHelper3.SetAttribute ("TrafficPattern", EnumValue (NocSyncApplication::DESTINATION_SPECIFIED));
  nocSyncAppHelper3.SetAttribute ("Destination", UintegerValue (7)); // destination
  nocSyncAppHelper3.SetAttribute ("MaxFlits", UintegerValue (packetLength));
  ApplicationContainer apps3 = nocSyncAppHelper3.Install (nodes.Get (5)); // source
  //  apps2.Start (Seconds (0.0));
  //  apps2.Stop (Seconds (10.0));

  NocSyncApplicationHelper nocSyncAppHelper4 (nodes, devs, hSize);
  nocSyncAppHelper4.SetAttribute ("InjectionProbability", DoubleValue (injectionProbability));
  nocSyncAppHelper4.SetAttribute ("TrafficPattern", EnumValue (NocSyncApplication::DESTINATION_SPECIFIED));
  nocSyncAppHelper4.SetAttribute ("Destination", UintegerValue (5)); // destination
  nocSyncAppHelper4.SetAttribute ("MaxFlits", UintegerValue (packetLength));
  ApplicationContainer apps4 = nocSyncAppHelper4.Install (nodes.Get (7)); // source
  //  apps2.Start (Seconds (0.0));
  //  apps2.Stop (Seconds (10.0));

  // Configure tracing of all enqueue, dequeue, and NetDevice receive events
  // Trace output will be sent to the noc-sync-test.tr file
  // Tracing should be kept disabled for big simulations
  NS_LOG_INFO ("Configure Tracing.");
  Ptr<OutputStreamWrapper> stream =
      Create<OutputStreamWrapper> ("noc-sync-test.tr", std::ios_base::binary | std::ios_base::out);
  noc->EnableAsciiAll (stream);

  //  GtkConfigStore configstore;
  //  configstore.ConfigureAttributes();

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();

  NS_LOG_INFO ("NoC dynamic power: " << noc->GetDynamicPower () << " W");
  NS_LOG_INFO ("NoC leakage power: " << noc->GetLeakagePower () << " W");
  NS_LOG_INFO ("NoC total power: " << noc->GetTotalPower () << " W");
  NS_LOG_INFO ("NoC area: " << noc->GetArea () << " um^2");

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
