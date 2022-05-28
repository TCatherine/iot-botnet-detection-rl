#include "sim.h"

NS_LOG_COMPONENT_DEFINE ("IOT_App");

IOTApplication::IOTApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	m_node = 0;
	m_packetSize = 0;
	m_env_port = 0;
	m_env = 0;
};

IOTApplication::~IOTApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void IOTApplication::Setup (uint16_t id, uint16_t iot_port, Ptr<Node> node, Ipv6Address local_address, 
	uint16_t local_port, uint32_t packetSize, uint32_t nPackets, DataRate dataRate){
		m_id = id;
		m_socket = Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());
		m_node = node;
		m_address = local_address;
		m_port = local_port;
		m_packetSize = packetSize;
		m_nPackets = nPackets;
		m_dataRate = dataRate;
		m_env_port = iot_port + id;		
}

void IOTApplication::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void IOTApplication::StartApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
							
	Inet6SocketAddress local = Inet6SocketAddress (m_address, m_port);
	m_socket->Bind (local);
	m_socket->Listen();
	m_socket->SetRecvCallback (MakeCallback (&IOTApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback(&IOTApplication::HandleSend, this));

	m_env = CreateObject<IotEnv> (m_id, Seconds(30));
	Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface> (m_env_port);
	m_env->SetOpenGymInterface(openGymInterface);
	m_env->m_application = this;
	
}

void IOTApplication::StopApplication ()
{  
	NS_LOG_FUNCTION_NOARGS ();

  if (m_env->m_sendEvent.IsRunning ()) {
      Simulator::Cancel (m_env->m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
	  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  m_socket = 0;
    }

	//Simulator::Cancel (m_env->m_sendEvent);
}

void IOTApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {}

uint32_t GetTime(Time time) {
	return (uint32_t)(time.GetSeconds());
}

void IOTApplication::PacketParse(Ptr<Packet> packet) {
	Ipv6Header ip_header;
	UdpHeader udp_header;
	packet->PeekHeader (udp_header);
	packet->PeekHeader (ip_header);
	Ipv6Address addr = ip_header.GetSourceAddress();

	auto lambda_find = [addr](struct packets_flow fl) -> bool {return fl.addrs == addr; } ;
	
	flow_it i;
	for(i = vector_packet_flow.begin(); i!= vector_packet_flow.end(); i++) {
		if (lambda_find(*i) == true) 
			break;
	}

	if (i == vector_packet_flow.end()) {
		packets_flow new_flow;
		new_flow.addrs = addr;
		new_flow.port = udp_header.GetSourcePort();
		new_flow.packet_number = 1;
		new_flow.sum_size_packet = packet->GetSize();
		new_flow.sum_ttl = ip_header.GetHopLimit();
		new_flow.s_time = Simulator::Now();
		new_flow.l_time = Simulator::Now();
		new_flow.l_interval = Seconds(0);
		new_flow.duration = Seconds(0);
		new_flow.sum_interval = Seconds(0);
		new_flow.max_deviation_interval = Seconds(0);
		vector_packet_flow.push_back(new_flow);
	}
	else {
		i->packet_number += 1;
		i->sum_size_packet = packet->GetSize();
		i->sum_ttl += ip_header.GetHopLimit();
		i->sum_interval += Simulator::Now() - i->l_time;
		i->l_interval = Simulator::Now() - i->l_time;
		i->duration = Simulator::Now() - i->s_time;
		i->l_time = Simulator::Now();
		uint16_t average_interval = GetTime(i->sum_interval);
		average_interval /= i->packet_number;
		if (abs(average_interval) - GetTime(i->max_deviation_interval) < 
			abs(average_interval) - GetTime(i->l_interval)) {
				i->max_deviation_interval = i->l_interval;  
			}
		}

		// for (it i = vector_packet_flow.begin(); i!=vector_packet_flow.end(); i++){
		// std::cout << i->addrs << std::endl;
		// std::cout << i->port << std::endl;
		// std::cout << i->packet_number << std::endl;
		// std::cout << i->sum_size_packet << std::endl;
		// std::cout << i->sum_ttl << std::endl;
		// std::cout << i->s_time << std::endl;
		// std::cout << i->l_time << std::endl;
		// std::cout << i->l_interval << std::endl;
		// std::cout << i->duration << std::endl;
		// std::cout << i->max_deviation_interval << std::endl;
		// std::cout << i->sum_interval << std::endl;
		// }

}

void IOTApplication::PacketFlowClear() {
	vector_packet_flow.clear();
	m_env->vector_features.clear();
	is_attack[m_id] = 0;
}

void IOTApplication::PacketFlowConvert() {
	for (flow_it i = vector_packet_flow.begin(); i != vector_packet_flow.end(); i++) {
		features f;

		i->addrs.GetBytes(f.addrs);
		// f.port = i->port;
		f.packet_number = i->packet_number;

		uint16_t n = i->packet_number;
		f.average_size_packet = i->sum_size_packet / n;
		f.average_ttl = i->sum_ttl / n;
		f.s_time = GetTime(i->s_time);
		f.l_time = GetTime(i->l_time);
		f.duration = GetTime(i->duration);
		f.average_interval = GetTime(i->sum_interval) / n;
		f.max_deviation_interval = GetTime(i->max_deviation_interval);

		m_env->vector_features.push_back(f);
	}

}

void IOTApplication::HandleRead (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;

	while (packet = socket->RecvFrom (from))
	{
		PacketParse(packet);
		Inet6SocketAddress remote = Inet6SocketAddress::ConvertFrom (from);

		UdpHeader udp_header;
		Ipv6Header ipv6_header;
		
		uint8_t *buffer = new uint8_t [m_packetSize];
		for (uint16_t i = 0; i < m_packetSize; i++)
			buffer[i] = (rand() % 0xFF) + 0x04;

		Ptr<Packet> p;
		p = new Packet(buffer, m_packetSize);
		p->AddHeader(udp_header);
		p->AddHeader(ipv6_header);

		m_socket->SendTo(p, 0, remote);
	}
}
