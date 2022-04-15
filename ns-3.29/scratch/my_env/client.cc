#include "sim.h"

NS_LOG_COMPONENT_DEFINE ("Client_App");

ClientApplication::ClientApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

ClientApplication::~ClientApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket->Close();
	m_socket = 0;
}

void ClientApplication::Setup (uint16_t id, Ptr<Node> node, Ipv6Address local_address, 
	uint16_t local_port, std::vector<Ipv6Address>& remote_address, 
	std::vector<uint16_t>& remote_port)  {
		m_id = id;
		m_node = node;
		m_socket = Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());
		m_local_address = local_address;
		m_local_port = local_port;
		m_remote_address = remote_address;
		m_remote_port = remote_port;

		uint8_t idx_np = rand() % clear_traffic.n_packets.size();
		uint8_t idx_ps = rand() % clear_traffic.packet_size.size();
		m_nPackets = clear_traffic.n_packets[idx_np];
		m_packetSize = clear_traffic.packet_size[idx_ps];

		SetStartTime(MilliSeconds(0));

		std::cout << "[Client " << m_id << "]  Setup: " << m_local_address << ":" << m_local_port << " " << m_interval << std::endl;
}

void ClientApplication::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}


void ClientApplication::StartApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();

	Inet6SocketAddress local = Inet6SocketAddress (m_local_address, m_local_port);
	m_socket->Bind (local);

	m_socket->SetRecvCallback (MakeCallback (&ClientApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback(&ClientApplication::HandleSend, this));

	std::cout << "[Client " << m_id << "]  Start Application" << std::endl;
	Schedule();
}

void ClientApplication::Schedule() {
	if (time_finish > Simulator::Now()) {
		m_interval = MilliSeconds(rand () % (30 * 200) + 100);
		Send();
		Simulator::Schedule (m_interval, &ClientApplication::Schedule, this);
	}
	else {
		uint16_t delay = rand() % 300;
		uint16_t work_time = rand() % 300 + 1;
		time_start = Seconds(delay) + Seconds(10);
		time_finish = Seconds(work_time) + time_start + Simulator::Now();
		m_interval = MilliSeconds(rand () % (work_time * 200) + 100);
		std::cout << "Client Application Pause [ " << time_start.GetMilliSeconds() << " millisec ]" << std::endl;
		Simulator::Schedule (time_start, &ClientApplication::Schedule, this);
	}
}

void ClientApplication::Send() {
	NS_LOG_FUNCTION (this);

	uint8_t* buffer = new uint8_t[m_packetSize];
	for (uint16_t i = 0; i < m_packetSize; i++){
		buffer[i] = (rand() % 0xFF) + 0x04;
	}

	uint16_t i = rand() % m_remote_address.size();
	Inet6SocketAddress remote = Inet6SocketAddress (m_remote_address[i], m_remote_port[i]);

	UdpHeader udp_header;
	Ipv6Header ipv6_header;
	ipv6_header.SetDestinationAddress(m_remote_address[i]);
	ipv6_header.SetSourceAddress(m_local_address);
	ipv6_header.SetPayloadLength(m_packetSize);
	ipv6_header.SetHopLimit(24);
	ipv6_header.SetNextHeader( Ipv6Header::IPV6_UDP);
	udp_header.SetDestinationPort(m_remote_port[i]);
	udp_header.SetSourcePort(m_local_port);
	
	Ptr<Packet> p;
	p = new Packet(buffer, m_packetSize);
	p->AddHeader(udp_header);
	p->AddHeader(ipv6_header);

	m_socket->Connect (remote);
	m_socket->SendTo(p, 0, remote);
	std::cout << "Client " << m_id << " -> IOT " << i << " [ delta = " 
		<< m_interval.GetMilliSeconds() << " millisec ]" << std::endl; 
}

void ClientApplication::StopApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();

	std::cout << "[Client " << m_id << "] Stop Application" << std::endl;
	m_socket->Close();
}

void ClientApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {}

void ClientApplication::HandleRead (Ptr<Socket> socket) {}

NS_OBJECT_ENSURE_REGISTERED (IOTApplication);