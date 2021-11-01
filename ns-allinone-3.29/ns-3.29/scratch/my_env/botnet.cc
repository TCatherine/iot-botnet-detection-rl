#include "sim.h"

NS_LOG_COMPONENT_DEFINE ("Botnet_App");

uint32_t botnet_attack_start;
uint32_t botnet_attack_finish;

BotApplication::BotApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

BotApplication::~BotApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void BotApplication::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void BotApplication::Setup (uint16_t id, Ptr<Socket> socket, Ipv6Address local_address, 
	uint16_t local_port, std::vector<Ipv6Address>& remote_address, 
	std::vector<uint16_t>& remote_port) {
		m_id = id;
		m_socket = socket;
		m_local_address = local_address;
		m_local_port = local_port;
		m_remote_address = remote_address;
		m_remote_port = remote_port;

		uint8_t idx_np = rand() % ddos_traffic.n_packets.size();
		uint8_t idx_ps = rand() % ddos_traffic.packet_size.size();
		m_nPackets = ddos_traffic.n_packets[idx_np];
		m_packetSize = ddos_traffic.packet_size[idx_ps];
		Time cur_time = Simulator::Now();
		uint32_t time_start = botnet_attack_start;
		uint32_t time_finish =  botnet_attack_finish;
		uint8_t interval = rand() % (time_finish - time_start) ;
		SetStartTime(Seconds(time_start) + cur_time);
		SetStopTime(Seconds(time_finish) + cur_time);
		
		m_interval = Seconds(interval);
		std::cout << "[Bot " << m_id << "]  Setup: " << m_local_address << ":" << m_local_port << " " << m_interval << std::endl;
		}

void BotApplication::StartApplication (void)
{	
	NS_LOG_FUNCTION_NOARGS ();
	recv_packets = 0;
	m_running = true;

	Inet6SocketAddress local = Inet6SocketAddress (m_local_address, m_local_port);
	m_socket->Bind (local);
	//m_socket->Listen();
	Ptr<Packet> p;
	uint8_t* buffer = new uint8_t[m_packetSize];
		for (uint16_t i =0; i < m_packetSize; i++){
		buffer[i] = random()%127+128;
	}

	p = new Packet(buffer, m_packetSize);
	
	for (uint16_t i = 0; i < m_remote_address.size(); i++){
		Inet6SocketAddress remote = Inet6SocketAddress (m_remote_address[i], m_remote_port[i]);
		//m_socket->SetAllowBroadcast(true);
		m_socket->Connect (remote);
		for (uint16_t n = 0; n < m_nPackets; n++)
			m_socket->SendTo(p, 0, remote);
	}

	m_socket->SetRecvCallback (MakeCallback (&BotApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback (&BotApplication::HandleSend, this));
		
	//m_socket->SendTo(p, 0, remote);
	std::cout << "[Bot " << m_id << "] Send from " << m_local_address << ":" << m_local_port << 
	" to Address: " << m_remote_address[0] << ":" << m_remote_port[0] << std::endl;
	 Simulator::Schedule (Seconds(14.0), &BotApplication::NextSend, this);
	//m_socket->Send(p);
}

void BotApplication::NextSend ()
{
  NS_LOG_FUNCTION (this);
  is_attack = 1;
	Ptr<Packet> p;
	uint8_t* buffer = new uint8_t[m_packetSize];
	for (uint16_t i =0; i < m_packetSize; i++){
		buffer[i] = (0x30+i) % 0x41 + 0x30;
	}

	p = new Packet(buffer, m_packetSize);

	for (uint16_t i = 0; i < m_remote_address.size(); i++){
		Inet6SocketAddress remote = Inet6SocketAddress (m_remote_address[i], m_remote_port[i]);
		//m_socket->SetAllowBroadcast(true);
		m_socket->Connect (remote);
		for (uint16_t n = 0; n < m_nPackets; n++)
			m_socket->SendTo(p, 0, remote);
	}
	Simulator::Schedule (m_interval, &BotApplication::NextSend, this);
}

void BotApplication::StopApplication ()
{
	is_attack = 0;
	NS_LOG_FUNCTION_NOARGS ();
	Time cur_time = Simulator::Now();
	uint32_t time_start = botnet_attack_start;
	uint32_t time_finish = botnet_attack_finish;
	uint32_t interval = rand() % ((time_finish - time_start) / 2);
	SetStartTime(Seconds(time_start) + cur_time);
	SetStopTime(Seconds(time_finish) + cur_time);
	m_interval = Seconds(interval);

	std::cout << "[Bot " << m_id << "] Stop Application" << std::endl;
}


void BotApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {
	//std::cout << "[Bot] Handle Send" << std::endl;
}


void BotApplication::HandleRead (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	//std::cout << "[Bot] Handle Read" << std::endl;
	while (packet = socket->RecvFrom (from))
	{
		uint8_t *buffer = new uint8_t [1024]; 
		packet->CopyData (buffer, 1024); 
		NS_LOG_INFO ("[Bot " << m_id << "] Received " << packet->GetSize () << 
		" bytes from client " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ());
		//InetSocketAddress remote = InetSocketAddress (m_remote_address, m_remote_port);
		//m_socket->SendTo(packet, 0, remote);
	}
}
NS_OBJECT_ENSURE_REGISTERED (IOTApplication);