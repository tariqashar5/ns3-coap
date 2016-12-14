/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "coapnode-helper.h"
#include "ns3/coap_node.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

CoapNodeHelper::CoapNodeHelper (uint16_t port)
{
  m_factory.SetTypeId (CoapNode::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void CoapNodeHelper::SetAttribute ( std::string name, const AttributeValue &value){
  m_factory.Set (name, value);
}

ApplicationContainer CoapNodeHelper::Install (Ptr<Node> node) const{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer CoapNodeHelper::Install (std::string nodeName) const{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer CoapNodeHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application> CoapNodeHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<CoapNode> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
