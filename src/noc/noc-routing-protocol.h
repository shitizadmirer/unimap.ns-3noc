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

#ifndef NOCROUTINGPROTOCOL_H_
#define NOCROUTINGPROTOCOL_H_

#include "ns3/object.h"
#include "ns3/mac48-address.h"
#include "noc-packet.h"
#include "noc-net-device.h"

namespace ns3
{

  class NocNetDevice;

  /**
   *
   * \brief Interface for the routing protocol used by NoC net devices
   *
   * Every routing protocol for NoCs must implement this interface. Each device (NocNetDevice) is supposed
   * to know of a single routing protocol to work with, see NocNetDevice::SetRoutingProtocol ().
   *
   */
  class NocRoutingProtocol : public Object
  {
  public:

    static TypeId
    GetTypeId();

    NocRoutingProtocol(std::string name);

    virtual
    ~NocRoutingProtocol();

    /**
     * Callback to be invoked when the route discovery procedure is completed.
     *
     * \param flag        indicating whether a route was actually found and all needed information is
     *                    added to the packet successfully
     *
     * \param packet      the NoC packet for which the route was resolved
     *
     * \param src         source address of the packet
     *
     * \param dst         destination address of the packet
     *
     * \param protocol    ethernet 'Protocol' field, needed to form a proper MAC-layer header
     *
     */
    typedef Callback<void,/* return type */
    bool, /* flag */
    Ptr<Packet> , /* packet */
    Mac48Address,/* src */
    Mac48Address,/* dst */
    uint16_t /* protocol */
    > RouteReplyCallback;

    /**
     * Request routing information, all packets must go through this request.
     *
     * Note that route discovery works async. -- RequestRoute returns immediately, while
     * reply callback will be called when routing information will be available.
     *
     * \return true if a valid route is already known
     * \param sourceIface the incoming interface of the packet
     * \param source        source address
     * \param destination   destination address
     * \param packet        the packet to be resolved (needed the whole packet, because
     *                      routing information is added as tags or headers). The packet
     *                      will be returned to reply callback.
     * \param protocolType  protocol ID, needed to form a proper MAC-layer header
     * \param routeReply    callback to be invoked after route discovery procedure, supposed
     *                      to really send packet using routing information.
     */
    virtual bool
    RequestRoute(uint32_t sourceIface, const Mac48Address source,
        const Mac48Address destination, Ptr<Packet> packet,
        uint16_t protocolType, RouteReplyCallback routeReply) = 0;

    /**
     * set the NoC net device to which this routing protocol is assigned to
     */
    void
    SetNocNetDevice(Ptr<NocNetDevice> nocNetDevice);

    /**
     * \return the NoC net device to which this routing protocol is assigned to
     */
    Ptr<NocNetDevice>
    GetNocNetDevice() const;

    /**
     * \return the name of this routing protocol
     */
    std::string
    GetName() const;

  protected:
    /**
     * the NoC net device to which this routing protocol is assigned to
     */
    Ptr<NocNetDevice> m_nocNetDevice;

  private:
    /**
     * the name of the routing protocol
     */
    std::string m_name;
  };

} // namespace ns3

#endif /* NOCROUTINGPROTOCOL_H_ */
