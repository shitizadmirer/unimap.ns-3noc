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

#include "irvine-router.h"
#include "ns3/log.h"
#include "ns3/noc-header.h"
#include "ns3/xy-routing.h"

NS_LOG_COMPONENT_DEFINE ("IrvineRouter");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED (IrvineRouter);

  TypeId
  IrvineRouter::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::IrvineRouter")
        .SetParent<NocRouter> ();
    return tid;
  }

  // we could easily name the router "Irvine router", but using __FILE__ should be more useful for debugging
  IrvineRouter::IrvineRouter() : NocRouter (__FILE__)
  {
    m_north1DeviceAdded = false;
    m_north2DeviceAdded = false;
    m_eastDeviceAdded = false;
    m_south1DeviceAdded = false;
    m_south2DeviceAdded = false;
    m_westDeviceAdded = false;
  }

  IrvineRouter::~IrvineRouter()
  {

  }

  bool
  IrvineRouter::RequestRoute(const Ptr<NocNetDevice> source, const Ptr<NocNode> destination,
      Ptr<Packet> packet, RouteReplyCallback routeReply)
  {
    NS_LOG_FUNCTION_NOARGS();
    GetRoutingProtocol()->RequestRoute (source, destination, packet, routeReply);
    return true;
  }

  Ptr<NocNetDevice>
  IrvineRouter::GetInjectionNetDevice (Ptr<NocPacket> packet, Ptr<NocNode> destination)
  {
    NS_LOG_FUNCTION_NOARGS();
    Ptr<NocNetDevice> netDevice;

    NocHeader nocHeader;
    packet->PeekHeader (nocHeader);

    // we don't really determine the correct input net device
    // but rather we determine the correct router (right or left)
    // based on whether the destination is at West or at East
    // from the source

    uint8_t xDistance = nocHeader.GetXDistance ();
    bool isEast = (xDistance & 0x08) != 0x08;
    if (!isEast)
      {
        netDevice = m_leftRouterInputDevices[0];
      }
    else
      {
        netDevice = m_rightRouterInputDevices[0];
      }
    NS_LOG_DEBUG ("Chosen injection net device is " << netDevice->GetAddress ());

    return netDevice;
  }

  /**
   * The device is added to the left or to the right router, based on its routing direction
   *
   * \see ns3::NocNetDevice#GetRoutingDirection()
   */
  uint32_t
  IrvineRouter::AddDevice (Ptr<NocNetDevice> device)
  {
    // we add all devices to m_devices as well
    uint32_t index = m_devices.size ();
    m_devices.push_back (device);
    NS_LOG_DEBUG ("Routing protocol is " << GetRoutingProtocol()->GetTypeId().GetName());
    // FIXME returns ns3::NocRoutingProtocol (not ns3::XyRouting)
//    if (GetRoutingProtocol()->GetTypeId().GetName().compare("ns3::XyRouting") == 0)
//      {
        switch (device->GetRoutingDirection ()) {
          case XyRouting::NONE:
            NS_LOG_WARN("The net device " << device->GetAddress () << " has no routing direction!");
            break;
          case XyRouting::NORTH:
            if (!m_north1DeviceAdded)
              {
                m_rightRouterInputDevices.push_back (device);
                m_rightRouterOutputDevices.push_back (device);
                m_north1DeviceAdded = true;
              }
            else
              {
                NS_ASSERT(!m_north2DeviceAdded);
                m_leftRouterInputDevices.push_back (device);
                m_leftRouterOutputDevices.push_back (device);
                m_north2DeviceAdded = true;
              }
            break;
          case XyRouting::EAST:
            NS_ASSERT(!m_eastDeviceAdded);
            m_leftRouterInputDevices.push_back (device);
            m_rightRouterOutputDevices.push_back (device);
            m_eastDeviceAdded = true;
            break;
          case XyRouting::SOUTH:
            if (!m_south1DeviceAdded)
              {
                m_rightRouterInputDevices.push_back (device);
                m_rightRouterOutputDevices.push_back (device);
                m_south1DeviceAdded = true;
              }
            else
              {
                NS_ASSERT(!m_south2DeviceAdded);
                m_leftRouterInputDevices.push_back (device);
                m_leftRouterOutputDevices.push_back (device);
                m_south2DeviceAdded = true;
              }
            break;
          case XyRouting::WEST:
            NS_ASSERT(!m_westDeviceAdded);
            m_rightRouterInputDevices.push_back (device);
            m_leftRouterOutputDevices.push_back (device);
            m_westDeviceAdded = true;
            break;
          default:
            break;
        }
//      }
//    else
//      {
//      NS_LOG_ERROR ("The Irvine router currently works only with XY routing!");
//      }
    return index;
  }

  Ptr<NocNetDevice>
  IrvineRouter::GetInputNetDevice (Ptr<NocNetDevice> sender, const int routingDirection)
  {
    NS_LOG_DEBUG ("Searching for an input net device for node " << GetNocNode ()->GetId ()
        << " and direction " << routingDirection);

    bool isRightIrvineRouter = isRightRouter (sender);
    bool isLeftIrvineRouter = isLeftRouter (sender);
    NS_ASSERT_MSG (isRightIrvineRouter || isLeftIrvineRouter, "The packet came through net device "
        << sender->GetAddress () << " This is not from right nor left router.");
    NS_ASSERT_MSG (!isRightIrvineRouter || !isLeftIrvineRouter, "The packet came through net device "
        << sender->GetAddress () << " This is from both right and left routers.");

    Ptr<NocNetDevice> netDevice = 0;
    // note that right and left routers correspond for north and south directions
    // but they don't for west and east (a west output from, a right router connects
    // to an east input, from a left router)
    if (isRightIrvineRouter)
      {
        NS_LOG_DEBUG ("The packet came through the right router");
        bool found = false;
        for (unsigned int i = 0; i < m_rightRouterInputDevices.size(); ++i)
          {
            Ptr<NocNetDevice> tmpNetDevice = m_rightRouterInputDevices[i]->GetObject<NocNetDevice> ();
            NS_LOG_DEBUG ("Right input " << tmpNetDevice->GetAddress ());
            if (tmpNetDevice->GetRoutingDirection () == routingDirection)
              {
                netDevice = tmpNetDevice;
                found = true;
                break;
              }
          }
        if (!found)
          {
            for (unsigned int i = 0; i < m_leftRouterInputDevices.size(); ++i)
              {
                Ptr<NocNetDevice> tmpNetDevice = m_leftRouterInputDevices[i]->GetObject<NocNetDevice> ();
                NS_LOG_DEBUG ("Left input " << tmpNetDevice->GetAddress ());
                if (tmpNetDevice->GetRoutingDirection () == routingDirection)
                  {
                    netDevice = tmpNetDevice;
                    found = true;
                    break;
                  }
              }
          }
      }
    else
      {
        NS_LOG_DEBUG ("The packet came through the left router");
        bool found = false;
        for (unsigned int i = 0; i < m_leftRouterInputDevices.size(); ++i)
          {
            Ptr<NocNetDevice> tmpNetDevice = m_leftRouterInputDevices[i]->GetObject<NocNetDevice> ();
            NS_LOG_DEBUG ("Left input " << tmpNetDevice->GetAddress ());
            if (tmpNetDevice->GetRoutingDirection () == routingDirection)
              {
                netDevice = tmpNetDevice;
                found = true;
                break;
              }
          }
        if (!found)
          {
            for (unsigned int i = 0; i < m_rightRouterInputDevices.size(); ++i)
              {
                Ptr<NocNetDevice> tmpNetDevice = m_rightRouterInputDevices[i]->GetObject<NocNetDevice> ();
                NS_LOG_DEBUG ("Right input " << tmpNetDevice->GetAddress ());
                if (tmpNetDevice->GetRoutingDirection () == routingDirection)
                  {
                    netDevice = tmpNetDevice;
                    found = true;
                    break;
                  }
              }
          }
      }
    if (netDevice)
      {
        NS_LOG_DEBUG ("Found net device " << netDevice->GetAddress ());
      }
    else
      {
        NS_LOG_DEBUG ("No net device found!");
      }
    return netDevice;
  }

  Ptr<NocNetDevice>
  IrvineRouter::GetOutputNetDevice (Ptr<NocNetDevice> sender, const int routingDirection)
  {
    NS_LOG_DEBUG ("Searching for an output net device for node " << GetNocNode ()->GetId ()
        << " and direction " << routingDirection << " (sender net device is " << sender->GetAddress () << ")");

    bool isRightIrvineRouter = isRightRouter (sender);
    bool isLeftIrvineRouter = isLeftRouter (sender);
    NS_ASSERT_MSG (isRightIrvineRouter || isLeftIrvineRouter, "The packet came through net device "
        << sender->GetAddress () << " This is not from right nor left router.");
    NS_ASSERT_MSG (!isRightIrvineRouter || !isLeftIrvineRouter, "The packet came through net device "
        << sender->GetAddress () << " This is from both right and left routers.");

    Ptr<NocNetDevice> netDevice = 0;
    if (isRightIrvineRouter)
      {
        NS_LOG_DEBUG ("The packet came through the right router");
        for (unsigned int i = 0; i < m_rightRouterOutputDevices.size(); ++i)
          {
            Ptr<NocNetDevice> tmpNetDevice = m_rightRouterOutputDevices[i]->GetObject<NocNetDevice> ();
            NS_LOG_DEBUG ("Right output " << tmpNetDevice->GetAddress ());
            if (tmpNetDevice->GetRoutingDirection () == routingDirection)
              {
                netDevice = tmpNetDevice;
//                break;
              }
          }
      }
    else
      {
        NS_LOG_DEBUG ("The packet came through the left router");
        for (unsigned int i = 0; i < m_leftRouterOutputDevices.size(); ++i)
          {
            Ptr<NocNetDevice> tmpNetDevice = m_leftRouterOutputDevices[i]->GetObject<NocNetDevice> ();
            NS_LOG_DEBUG ("Left output " << tmpNetDevice->GetAddress ());
            if (tmpNetDevice->GetRoutingDirection () == routingDirection)
              {
                netDevice = tmpNetDevice;
//                break;
              }
          }
      }
    if (netDevice)
      {
        NS_LOG_DEBUG ("Found net device " << netDevice->GetAddress ());
      }
    else
      {
        NS_LOG_DEBUG ("No net device found!");
      }
    return netDevice;
  }

  bool
  IrvineRouter::isRightRouter (Ptr<NocNetDevice> sender)
  {
    bool isRightRouter = false;
    Ptr<IrvineRouter> router = sender->GetNode ()->GetObject<NocNode> ()->GetRouter ()->GetObject<IrvineRouter> ();
    for (unsigned int i = 0; i < router->m_rightRouterInputDevices.size(); ++i)
      {
        NS_LOG_DEBUG ("Comparing " << router->m_rightRouterInputDevices[i]->GetAddress ()
            << " with " << sender->GetAddress ());
        if (router->m_rightRouterInputDevices[i] == sender)
          {
            isRightRouter = true;
            break;
          }
      }
    NS_LOG_DEBUG ("Comparison result: " << isRightRouter);
    return isRightRouter;
  }

  bool
  IrvineRouter::isLeftRouter (Ptr<NocNetDevice> sender)
  {
    bool isLeftRouter = false;
    Ptr<IrvineRouter> router = sender->GetNode ()->GetObject<NocNode> ()->GetRouter ()->GetObject<IrvineRouter> ();
    for (unsigned int i = 0; i < router->m_leftRouterInputDevices.size(); ++i)
      {
      NS_LOG_DEBUG ("Comparing " << router->m_leftRouterInputDevices[i]->GetAddress ()
          << " with " << sender->GetAddress ());
        if (router->m_leftRouterInputDevices[i] == sender)
          {
            isLeftRouter = true;
            break;
          }
      }
    NS_LOG_DEBUG ("Comparison result: " << isLeftRouter);
    return isLeftRouter;
  }

} // namespace ns3
