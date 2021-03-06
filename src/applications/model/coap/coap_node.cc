/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2016 uc3m
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
 */

#include "coap_node.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("CoapNodeApplication");

NS_OBJECT_ENSURE_REGISTERED(CoapNode);

TypeId CoapNode::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::CoapNode")
                      .SetParent<Application> ()
                      .SetGroupName("Applications")
                      .AddConstructor<CoapNode> ()
                      .AddAttribute("Port", "Port on which we listen for incoming packets.",
                                    UintegerValue(COAP_DEFAULT_PORT),
                                    MakeUintegerAccessor(&CoapNode::m_port),
                                    MakeUintegerChecker<uint16_t> ())
                      .AddAttribute("mcast", "Should I answer in multicast mode?",
                                    UintegerValue(0),
                                    MakeUintegerAccessor(&CoapNode::m_mcast),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("startDelay", "Delay when you start the app",
                                    UintegerValue(30),
                                    MakeUintegerAccessor(&CoapNode::m_startDelay),
                                    MakeUintegerChecker<uint32_t>())
                      .AddAttribute("interval", "The time to wait between packets. If it is 0-> Server Mode.",
                                    TimeValue(Seconds(90.0)),
                                    MakeTimeAccessor(&CoapNode::m_interval),
                                    MakeTimeChecker())
                      .AddAttribute("nPackets", "The maximum number of packets the application will send",
                                    UintegerValue(0),
                                    MakeUintegerAccessor(&CoapNode::m_count),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("cache", "If 1, cache is sent with answers, If 0, cache is not send.",
                                    UintegerValue(1),
                                    MakeUintegerAccessor(&CoapNode::m_cacheopt),
                                    MakeUintegerChecker<uint16_t> ())
                      .AddAttribute("cacheInterval", "Interval in which the cache is showed and checked",
                                    UintegerValue(30),
                                    MakeUintegerAccessor(&CoapNode::m_cacheinterval),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("useMaxAge", "Delete items after max-Age? If >0 it is the age given to the services the node has",
                                    UintegerValue(60),
                                    MakeUintegerAccessor(&CoapNode::m_ageTime),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("mDNS", "If it is 1, coAP uses discovery to detect services.",
                                    UintegerValue(0),
                                    MakeUintegerAccessor(&CoapNode::m_activatemDns),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("stime", "If it is 1, it uses smart tx time to save transmissions, if it is 2, it retransmitt just untx heared services.",
                                    UintegerValue(0),
                                    MakeUintegerAccessor(&CoapNode::m_stime),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddAttribute("ping", "If it is 1, coAP send a ping when he shows his cache entries",
                                    UintegerValue(0),
                                    MakeUintegerAccessor(&CoapNode::m_activatePing),
                                    MakeUintegerChecker<uint32_t> ())
                      .AddTraceSource("Tx", "A new packet is created and is sent",
                                      MakeTraceSourceAccessor(&CoapNode::m_txTrace),
                                      "ns3::Packet::TracedCallback")
  ;

  return tid;
}

CoapNode::CoapNode() {
  NS_LOG_FUNCTION(this);
  m_socket    = 0;
  m_sendEvent = EventId();
  m_data      = 0;
  m_dataType  = 0;

  // Ptr<UniformRandomVariable> x = ;
  m_mcastAddr.Set(COAP_MCAST_ADDR);
}

CoapNode::~CoapNode() {
  NS_LOG_FUNCTION(this);
  m_socket  = 0;
  m_socket6 = 0;
  delete[] m_data;
  m_data     = 0;
  m_dataType = 0;
}

void CoapNode::DoDispose(void) {
  NS_LOG_FUNCTION(this);
  Application::DoDispose();
}

void CoapNode::StartApplication(void) {
  NS_LOG_FUNCTION(this);

  readServicesFile();

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    m_socket->Bind(local);

    if (addressUtils::IsMulticast(m_local)) {
      NS_LOG_INFO("Joining mcast group...");
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);

      if (udpSocket)
      {
        // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup(0, m_local);
      }
      else
      {
        NS_FATAL_ERROR("Error: Failed to join multicast group");
      }
    }
  }

  if (m_socket6 == 0)
  {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket6 = Socket::CreateSocket(GetNode(), tid);
    Inet6SocketAddress local6 = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
    m_socket6->Bind(local6);

    if (addressUtils::IsMulticast(local6))
    {
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket6);

      if (udpSocket)
      {
        // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup(0, local6);
      }
      else
      {
        NS_FATAL_ERROR("Error: Failed to join multicast group");
      }
    }
  }

  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  m_dnssocket = Socket::CreateSocket(GetNode(), tid);
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), MDNS_SOURCE_PORT);
  m_dnssocket->Bind(local);

  if (addressUtils::IsMulticast(m_local)) {
    NS_LOG_INFO("Joining mcast group...");
    Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_dnssocket);

    if (udpSocket)
    {
      // equivalent to setsockopt (MCAST_JOIN_GROUP)
      udpSocket->MulticastJoinGroup(0, m_local);
    }
    else
    {
      NS_FATAL_ERROR("Error: Failed to join multicast group");
    }
  }
  m_dnssocket->SetAllowBroadcast(true);

  m_socket->SetAllowBroadcast(true);
  m_socket->SetRecvCallback(MakeCallback(&CoapNode::HandleRead, this));
  m_socket6->SetRecvCallback(MakeCallback(&CoapNode::HandleRead, this));
  m_dnssocket->SetRecvCallback(MakeCallback(&CoapNode::HandleDns, this));

  // ScheduleTransmit (Seconds (0.));

  if (m_interval != 0) {
    ScheduleTransmit(Seconds(Uniform(m_startDelay)));
  }

  if (m_count != 0) {
    m_petitionLimit = true;
  } else {
    m_petitionLimit = false;
  }

  m_showCache = Simulator::Schedule(Seconds(Uniform(m_cacheinterval)), &CoapNode::showCache, this);
}

void CoapNode::StopApplication() {
  NS_LOG_FUNCTION(this);

  if (m_socket != 0) {
    m_socket->Close();
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
  }

  if (m_socket6 != 0) {
    m_socket6->Close();
    m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
  }

  if (m_dnssocket != 0) {
    m_dnssocket->Close();
    m_dnssocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
  }
  m_sendEvent.Cancel(); // !< Event to send the next packet
  m_showCache.Cancel();
}

void CoapNode::HandleRead(Ptr<Socket>socket) {
  NS_LOG_FUNCTION(this << socket);
  recvDtg(socket);
}

void CoapNode::HandleDns(Ptr<Socket>socket) {
  NS_LOG_FUNCTION(this << socket);
  Address from;
  Ptr<Packet> dnspacket;

  while ((dnspacket = socket->RecvFrom(from))) {
    uint32_t data_size = dnspacket->GetSize();

    MDns my_mdns(m_dnssocket, m_txTrace);
    int  res = my_mdns.recvdns(dnspacket, from);

    if (res == 1) { // Recibe Query
      if (InetSocketAddress::IsMatchingType(from)) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << GetAddr() << " RECV " << data_size << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " DNS QUERY ID:" << my_mdns.mDNSId);
      }

      if (m_stime != 0) { // DELAYED RESPONSE CODE
        if (checkID(my_mdns.mDNSId) == PKT_NOTFOUND) {
          uint64_t dtime               = getResponseTime();
          Query    delayed_query       = my_mdns.queries[0];
          EventId  m_sendDiscoResponse = Simulator::Schedule(MilliSeconds(dtime), &CoapNode::sendMDnsCache, this, delayed_query, from, my_mdns.mDNSId);
          addID(my_mdns.mDNSId, m_sendDiscoResponse);
          NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << GetAddr() << " DELAY ANSWER ID " << my_mdns.mDNSId << " TO " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " " << dtime << " ms");
        }
      } else {
        sendMDnsCache(my_mdns.queries[0], from, my_mdns.mDNSId);
      }

      // my_mdns.Clear();
    }

    if (res == 2) { // Recibe Answer, tiene que procesar la Query
      if (InetSocketAddress::IsMatchingType(from)) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << GetAddr() << " RECV " << data_size << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " DNS ANSWER ID:" << my_mdns.mDNSId);
      }

      if (m_stime == ALL_OR_NOTHING) { // Borramos el ID ya que ha sido contestado con anterioridad
        delID(my_mdns.mDNSId);
      }

      // NS_LOG_INFO("DEBUG ANSW LENGTH " << my_mdns.answers.size());
      for (u_int32_t i = 0; i < my_mdns.answers.size(); ++i) {
        const Answer answer = my_mdns.answers[i];

        if (answer.valid) {
          std::string puri(answer.rdata_buffer);
          Ipv4Address mdip(split(puri, '/')[0].c_str());
          std::string vser = split(puri, '/')[1];

          if (m_stime == PARTIAL_SELECTIVE) {
            updateID(my_mdns.mDNSId, mdip, vser);
          }

          if (addEntry(mdip, vser, (uint32_t)answer.rrttl) == false) {
            if (mdip == InetSocketAddress::ConvertFrom(from).GetIpv4()) {
              updateEntry(mdip, vser, (uint32_t)answer.rrttl);
            }
          }

          // NS_LOG_INFO("\t|-> ANSW: "<< answer.name_buffer<<" = "<< answer.rdata_buffer);
        }
      }
      deleteOutdated();
    }
  }
}

/*
 * It Schedules the transmission of the next packet request for discovery of the actual network
 */
void CoapNode::ScheduleTransmit(Time dt) {
  NS_LOG_FUNCTION(this << dt);

  if (m_petitionLimit == true) {
    if (m_count != 0) {
      // m_sendEvent = Simulator::Schedule (dt, &CoapNode::Send, this,COAP_DISCOVERY_GET,NEW_DTG_ID);
      if (m_activatemDns == 1) {
        m_sendEvent = Simulator::Schedule(dt, &CoapNode::sendmDnsRequest, this);
      } else {
        m_sendEvent = Simulator::Schedule(dt, &CoapNode::SendDiscovery, this);
      }
      m_count = m_count - 1;
    }
  } else {
    if (m_activatemDns == 1) {
      m_sendEvent = Simulator::Schedule(dt, &CoapNode::sendmDnsRequest, this);
    } else {
      m_sendEvent = Simulator::Schedule(dt, &CoapNode::SendDiscovery, this);
    }
  }
}

int CoapNode::parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
  uint8_t *p = *buf;
  uint8_t  headlen = 1;
  uint16_t len, delta;

  if (buflen < headlen) return -1;

  delta = (p[0] & 0xF0) >> 4;
  len   = p[0] & 0x0F;

  if (delta == 13) {
    headlen++;

    if (buflen < headlen) return -1;

    delta = p[1] + 13;
    p++;
  } else if (delta == 14) {
    headlen += 2;

    if (buflen < headlen) return -1;

    delta = ((p[1] << 8) | p[2]) + 269;
    p    += 2;
  } else if (delta == 15) return -1;

  if (len == 13) {
    headlen++;

    if (buflen < headlen) return -1;

    len = p[1] + 13;
    p++;
  } else if (len == 14) {
    headlen += 2;

    if (buflen < headlen) return -1;

    len = ((p[1] << 8) | p[2]) + 269;
    p  += 2;
  } else if (len == 15) return -1;

  if ((p + 1 + len) > (*buf + buflen)) return -1;

  option->number  = delta + *running_delta;
  option->buffer  = p + 1;
  option->length  = len;
  *buf            = p + 1 + len;
  *running_delta += delta;

  return 0;
}

void CoapNode::readServicesFile() {
  std::ifstream file("/home/semaesoj/config.csv"); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
  std::string   value = "";

  while (file.good()) {
    getline(file, value, ','); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
    value = value.substr(0, value.size() - 1);

    // NS_LOG_INFO(value); // display value removing the first and the last character from it
  }
}

void CoapNode::saveLog(std::string texttolog) {
  std::string ipaddrstr                = Ipv4AddressToString(GetAddr());
  std::string node_id                  = split(ipaddrstr, ';')[3];
  Ptr<OutputStreamWrapper> cacheStream = Create<OutputStreamWrapper>(node_id + "_log.txt", std::ios::app);
  std::ostream *stream                 = cacheStream->GetStream();
  *stream << texttolog << std::endl;
}
} // Namespace ns3
