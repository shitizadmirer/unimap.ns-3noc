/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010
 *               Advanced Computer Architecture and Processing Systems (ACAPS),
 *               Lucian Blaga University of Sibiu, Romania
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

#include "noc-mesh-2d.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/xy-routing.h"
#include "ns3/4-way-router.h"
#include "ns3/irvine-router.h"
#include "ns3/saf-switching.h"
#include "ns3/wormhole-switching.h"
#include "ns3/vct-switching.h"
#include "ns3/noc-packet-tag.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("NocMesh2D");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (NocMesh2D);

  TypeId
  NocMesh2D::GetTypeId ()
  {
    static TypeId tid = TypeId("ns3::NocMesh2D")
        .SetParent<NocTopology> ()
        .AddConstructor<NocMesh2D> ()
        .AddAttribute ("hSize",
            "how many nodes the 2D mesh will have on one horizontal line",
            UintegerValue (4),
            MakeUintegerAccessor (&NocMesh2D::m_hSize),
            MakeUintegerChecker<uint32_t> (1));
    return tid;
  }

  NocMesh2D::NocMesh2D () : NocTopology ()
  {
    NS_LOG_FUNCTION_NOARGS ();
  }

  NocMesh2D::~NocMesh2D ()
  {
    NS_LOG_FUNCTION_NOARGS ();
  }

  NetDeviceContainer
  NocMesh2D::Install (NodeContainer nodes)
  {
    NS_LOG_FUNCTION_NOARGS ();
    NS_LOG_DEBUG ("hSize " << m_hSize);

    Ptr<NocChannel> channel = 0;
    Ptr<NocNetDevice> netDevice;

    for (unsigned int i = 0; i < nodes.GetN (); ++i)
      {
        Ptr<NocNode> nocNode = nodes.Get (i)->GetObject<NocNode> ();
        if (nocNode->GetRouter () == 0)
          {
            Ptr<NocRouter> router = m_routerFactory.Create ()->GetObject<NocRouter> ();
            router->SetNocNode (nocNode);
            nocNode->SetRouter (router);
            Ptr<NocRoutingProtocol> routingProtocol = m_routingProtocolFactory.Create ()->GetObject<NocRoutingProtocol> ();
            router->SetRoutingProtocol (routingProtocol);
            Ptr<NocSwitchingProtocol> switchingProtocol =
                m_switchingProtocolFactory.Create ()->GetObject<NocSwitchingProtocol> ();
            router->SetSwitchingProtocol (switchingProtocol);
          }
      }

    // create the horizontal channels (and net devices)
    for (unsigned int i = 0; i < nodes.GetN (); ++i)
      {
        Ptr<NocNode> nocNode = nodes.Get (i)->GetObject<NocNode> ();
        if (channel != 0)
          {
            netDevice = CreateObject<NocNetDevice> ();
            netDevice->SetAddress (Mac48Address::Allocate ());
            netDevice->SetChannel (channel);
            netDevice->SetRoutingDirection (NocRoutingProtocol::WEST);
            m_devices.Add (netDevice);
            netDevice->SetNocTopology (this);
            // attach input buffering (we don't use output buffering for the moment)
            Ptr<Queue> inQueue = m_inQueueFactory.Create<Queue> ();
            netDevice->SetInQueue (inQueue);
            nocNode->AddDevice (netDevice);
            nocNode->GetRouter ()->AddDevice (netDevice);
          }

        if (i == 0 || (i > 0 && (i + 1) % m_hSize != 0))
          {
            channel = m_channelFactory.Create ()->GetObject<NocChannel> ();
            netDevice = CreateObject<NocNetDevice> ();
            netDevice->SetAddress (Mac48Address::Allocate ());
            netDevice->SetChannel (channel);
            netDevice->SetRoutingDirection (NocRoutingProtocol::EAST);
            m_devices.Add (netDevice);
            netDevice->SetNocTopology (this);
            // attach input buffering (we don't use output buffering for the moment)
            Ptr<Queue> inQueue = m_inQueueFactory.Create<Queue> ();
            netDevice->SetInQueue (inQueue);
            nocNode->AddDevice (netDevice);
            nocNode->GetRouter ()->AddDevice (netDevice);
          }
        else
          {
            channel = 0;
          }
      }

    // create the vertical channels (and net devices)
    channel = 0;
    std::vector<Ptr<NocChannel> > columnChannels (m_hSize);
    for (unsigned int i = 0; i < nodes.GetN (); i = i + m_hSize)
      {
        for (unsigned int j = 0; j < m_hSize; ++j)
          {
            Ptr<NocNode> nocNode = nodes.Get (i + j)->GetObject<NocNode> ();
            if (columnChannels[j] != 0)
              {
                channel = columnChannels[j];
                netDevice = CreateObject<NocNetDevice> ();
                netDevice->SetAddress (Mac48Address::Allocate ());
                netDevice->SetChannel (channel);
                netDevice->SetRoutingDirection (NocRoutingProtocol::NORTH);
                m_devices.Add (netDevice);
                netDevice->SetNocTopology (this);
                // attach input buffering (we don't use output buffering for the moment)
                Ptr<Queue> inQueue = m_inQueueFactory.Create<Queue> ();
                netDevice->SetInQueue (inQueue);
                nocNode->AddDevice (netDevice);
                nocNode->GetRouter ()->AddDevice (netDevice);
              }
            if (i < nodes.GetN () - m_hSize)
              {
                channel = m_channelFactory.Create ()->GetObject<NocChannel> ();
                netDevice = CreateObject<NocNetDevice> ();
                netDevice->SetAddress (Mac48Address::Allocate ());
                netDevice->SetChannel (channel);
                netDevice->SetRoutingDirection (NocRoutingProtocol::SOUTH);
                m_devices.Add (netDevice);
                netDevice->SetNocTopology (this);
                // attach input buffering (we don't use output buffering for the moment)
                Ptr<Queue> inQueue = m_inQueueFactory.Create<Queue> ();
                netDevice->SetInQueue (inQueue);
                nocNode->AddDevice (netDevice);
                nocNode->GetRouter ()->AddDevice (netDevice);
                columnChannels[j] = channel;
              }
            else
              {
                columnChannels[j] = 0;
              }
          }
      }

    NS_LOG_DEBUG ("Printing the 2D mesh topology (channels <-> net devices <-> nodes)...");
    for (uint32_t i = 0; i < m_devices.GetN (); ++i)
      {
        Ptr<NetDevice> device = m_devices.Get (i);
        NS_LOG_DEBUG ("\tNode " << device->GetNode()->GetId() <<
            " has a net device with (MAC) address " << device->GetAddress() <<
            " connected to channel " << device->GetChannel()->GetId());
      }
    NS_LOG_DEBUG ("Done with printing the 2D mesh topology.");

    return m_devices;
  }

  void
  NocMesh2D::SaveTopology (NodeContainer nodes, string directoryPath)
  {
    NS_LOG_FUNCTION (directoryPath);

    for (uint32_t i = 0; i < nodes.GetN (); ++i)
      {
        Ptr<Node> node = nodes.Get(i);
        uint32_t nodeId = node->GetId ();

        ofstream nodeXml;
        stringstream ss;
        ss << directoryPath << FILE_SEPARATOR << m_hSize << "x" << nodes.GetN () / m_hSize << FILE_SEPARATOR << "nodes"
            << FILE_SEPARATOR << "node-" << i << ".xml";
        string nodeXmlFilePath = ss.str ();
        NS_LOG_DEBUG ("Saving node " << nodeId << " in " << nodeXmlFilePath);
        nodeXml.open (nodeXmlFilePath.c_str (), ios::out);

        ss.str ("");
        ss << i;
        string nodeIdAsString = ss.str ();
        try
          {
            nodeType::id_type idType (nodeIdAsString);
            nodeType theNodeType (idType);

            research::noc::application_mapping::unified_framework::schema::node::node (nodeXml, theNodeType);
          }
        catch (const xml_schema::exception& e)
          {
            NS_LOG_ERROR (e);
          }

        nodeXml.close ();
      }
  }

} // namespace ns3

