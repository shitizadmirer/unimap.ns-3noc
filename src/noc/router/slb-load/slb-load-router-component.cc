/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Systems and Networking, University of Augsburg, Germany
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
 * Author: Ciprian Radu <radu@informatik.uni-augsburg.de>
 */

#include "ns3/log.h"
#include "slb-load-router-component.h"

NS_LOG_COMPONENT_DEFINE ("SlbLoadRouterComponent");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (SlbLoadRouterComponent);

  SlbLoadRouterComponent::SlbLoadRouterComponent() : LoadRouterComponent (__FILE__)
  {
    ;
  }

  TypeId
  SlbLoadRouterComponent::GetTypeId ()
  {
    static TypeId tid = TypeId("ns3::SlbLoadRouterComponent")
        .SetParent<LoadRouterComponent> ()
        .AddConstructor<SlbLoadRouterComponent> ();
    return tid;
  }

  SlbLoadRouterComponent::~SlbLoadRouterComponent ()
  {
    ;
  }

  int
  SlbLoadRouterComponent::GetLocalLoad ()
  {
    int load = 0;
    // load = (int)((router.getNewLoad() / (8.0f * (6.0f * router.getDataFlitSpeedup() + router.getNode().getProcessingElement().getMessageLength()))) * 100.0f);
    int dataFlitSpeedup = 2; // FIXME
    int messageLength = 9; // FIXME
    load = (int) ((m_load / (8.0 * (6.0 * dataFlitSpeedup + messageLength))) * 100);

    NS_ASSERT (load >= 0 && load <= 100);

    return load;
  }

  int
  SlbLoadRouterComponent::GetLoadForDirection (Ptr<NocNetDevice> sourceDevice, Ptr<NocNetDevice> selectedDevice)
  {
    int load = GetLocalLoad ();
    double neighbourLoad = 0;
    int counter = 0;

    Ptr<NocRouter> router = sourceDevice->GetNode ()->GetObject<NocNode> ()->GetRouter ();
    NS_ASSERT (router);
    if (selectedDevice->GetRoutingDirection () != NocRoutingProtocol::NORTH)
      {
        neighbourLoad += router->GetNeighborLoad (sourceDevice, NocRoutingProtocol::NORTH);
        counter++;
      }
    if (selectedDevice->GetRoutingDirection () != NocRoutingProtocol::EAST)
      {
        neighbourLoad += router->GetNeighborLoad (sourceDevice, NocRoutingProtocol::EAST);
        counter++;
      }
    if (selectedDevice->GetRoutingDirection () != NocRoutingProtocol::SOUTH)
      {
        neighbourLoad += router->GetNeighborLoad (sourceDevice, NocRoutingProtocol::SOUTH);
        counter++;
      }
    if (selectedDevice->GetRoutingDirection () != NocRoutingProtocol::WEST)
      {
        neighbourLoad += router->GetNeighborLoad (sourceDevice, NocRoutingProtocol::WEST);
        counter++;
      }
    if (counter != 0)
      {
        neighbourLoad /= counter;
        load = (2 * load + neighbourLoad) / 3;
      }

    return load;
  }

} // namespace ns3
