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

#ifndef IRVINELOADROUTER_H_
#define IRVINELOADROUTER_H_

#include "ns3/irvine-router.h"
#include "ns3/load-router-component.h"

namespace ns3
{

  /**
   * \brief Irvine router with load information
   */
  class IrvineLoadRouter : public IrvineRouter
  {
  public:

    static TypeId
    GetTypeId ();

    /**
     * Default constructor
     *
     * Note that you must specify a LoadRouterComponent manually when using this constructor
     *
     *
     * \see #SetLoadComponent(Ptr<LoadRouterComponent> loadComponent)
     */
    IrvineLoadRouter ();

    /**
     * Constructor
     *
     * \param loadComponent the load router component
     */
    IrvineLoadRouter (Ptr<LoadRouterComponent> loadComponent);

    virtual
    ~IrvineLoadRouter ();

    virtual void
    SetLoadComponent (Ptr<LoadRouterComponent> loadComponent);

    /**
     * Adds the load received from a neighbor router (marked by its topological direction)
     *
     * \param load the load information received from a neighbor
     * \param sourceDevice the net device from which the load information came
     */
    void
    AddNeighborLoad (int load, Ptr<NocNetDevice> sourceDevice);

  private:

  };

} // namespace ns3

#endif /* IRVINELOADROUTER_H_ */
