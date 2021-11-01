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
	m_socket = 0;
}

void ClientApplication::Setup (uint16_t id, Ptr<Socket> socket, Ipv6Address local_address, 
	uint16_t local_port, std::vector<Ipv6Address>& remote_address, 
	std::vector<uint16_t>& remote_port){
		m_id = id;
		m_socket = socket;
		m_local_address = local_address;
		m_local_port = local_port;
		m_remote_address = remote_address;
		m_remote_port = remote_port;

		uint8_t idx_np = rand() % clear_traffic.n_packets.size();
		uint8_t idx_ps = rand() % clear_traffic.packet_size.size();
		m_nPackets = clear_traffic.n_packets[idx_np];
		m_packetSize = clear_traffic.packet_size[idx_ps];
		uint8_t time_start = rand() % 100 + 1;
		uint8_t time_finish = rand() % 1000 + time_start;
		uint8_t interval = rand() % (time_finish - time_start) ;
		SetStartTime(Seconds(time_start));
		SetStopTime(Seconds(time_finish));
		m_interval = Seconds(interval);
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
	recv_packets = 0;
	m_running = true;

	Inet6SocketAddress local = Inet6SocketAddress (m_local_address, m_local_port);
	m_socket->Bind (local);
	//m_socket->Listen();
	Ptr<Packet> p;
	std::cout << "Size Packet" << m_packetSize << std::endl;
	uint8_t* buffer = new uint8_t[m_packetSize];
		for (uint16_t i =0; i < m_packetSize; i++){
		buffer[i] = random()%252+4;
	}

	p = new Packet(buffer, m_packetSize);
	
	for (uint16_t i = 0; i < m_remote_address.size(); i++){
		Inet6SocketAddress remote = Inet6SocketAddress (m_remote_address[i], m_remote_port[i]);
		//m_socket->SetAllowBroadcast(true);
		m_socket->Connect (remote);
		for (uint16_t n = 0; n < m_nPackets; n++)
			m_socket->SendTo(p, 0, remote);
	}

	m_socket->SetRecvCallback (MakeCallback (&ClientApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback(&ClientApplication::HandleSend, this));
		
	//m_socket->SendTo(p, 0, remote);
	std::cout << "[Client " << m_id << "] Send from " << m_local_address << ":" << m_local_port << std::endl;
	 Simulator::Schedule (m_interval, &ClientApplication::NextSend, this);
	//m_socket->Send(p);
	}

void ClientApplication::NextSend ()
{
	NS_LOG_FUNCTION (this);
	Ptr<Packet> p;
	uint8_t* buffer = new uint8_t[m_packetSize];
	for (uint16_t i = 0; i < m_packetSize; i++){
		buffer[i] = (rand() % 0x4E) + 0x30;
	}

	p = new Packet(buffer, m_packetSize);

	for (uint16_t i = 0; i < m_remote_address.size(); i++){
		Inet6SocketAddress remote = Inet6SocketAddress (m_remote_address[i], m_remote_port[i]);
		//m_socket->SetAllowBroadcast(true);
		m_socket->Connect (remote);
		for (uint16_t n = 0; n < m_nPackets; n++)
			m_socket->SendTo(p, 0, remote);
	}
	Simulator::Schedule (m_interval, &ClientApplication::NextSend, this);
}

void ClientApplication::StopApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	Time cur_time = Simulator::Now();
	uint32_t time_start = rand() % 100 + 4;
	uint32_t time_finish = rand() % 1000 + time_start;
	uint32_t interval = rand() % (time_finish - time_start);
	SetStartTime(Seconds(time_start)+cur_time);
	SetStopTime(Seconds(time_finish)+cur_time);
	m_interval = Seconds(interval);

	std::cout << "[Client " << m_id << "] Stop Application" << std::endl;
	//Simulator::Schedule (Seconds(time_start), &ClientApplication::NextSend, this);
	// if (m_socket != 0) 
	// {
	// 	m_socket->Close ();
	// 	m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	// }
}

void ClientApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {
	//std::cout << "[Client] Handle Send" << std::endl;
}

void ClientApplication::HandleRead (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	//std::cout << "[Client] Handle Read" << std::endl;
	while (packet = socket->RecvFrom (from))
	{
		if (InetSocketAddress::IsMatchingType (from))
		{
		
			uint8_t *buffer = new uint8_t [1024]; 
			packet->CopyData (buffer, 1024); 
			NS_LOG_INFO ("[Client " << m_id << "]  Received " << packet->GetSize () << " bytes from client " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " with contents " <<buffer);
		}
	}
}
NS_OBJECT_ENSURE_REGISTERED (IOTApplication);