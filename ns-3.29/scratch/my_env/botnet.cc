#include "sim.h"

NS_LOG_COMPONENT_DEFINE ("Botnet_App");

botnet_timer* timer = nullptr;

struct time_parameters {
	uint16_t max_interval_ms = 60;
	uint16_t min_interval_ms = 10;
	uint16_t max_delay_s = 120;
	uint16_t min_delay_s = 60;
	uint16_t max_work_time_s = 60;
	uint16_t min_work_time_s = 30;
};

time_parameters p;

void botnet_timer_setup() {
	if (timer == nullptr) {
		timer = new botnet_timer;
		timer->trigger = false;
	}

	if (timer->trigger)
		return;

	uint16_t delay = rand() % p.max_delay_s + p.min_delay_s;
	uint16_t work_time = rand() % p.max_work_time_s + p.min_work_time_s;
	timer->start = Seconds(delay) + Simulator::Now();
	timer->finish = Seconds(work_time) + timer->start;
	timer->interval = MilliSeconds(rand () % p.max_interval_ms + p.min_interval_ms);
	timer->trigger = true;	
}

BotApplication::BotApplication() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

BotApplication::~BotApplication() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void BotApplication::DoDispose (void) {
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void BotApplication::Setup (uint16_t id, Ptr<Node> node, Ipv6Address local_address, 
	uint16_t local_port, std::vector<Ipv6Address>& remote_address, 
	std::vector<uint16_t>& remote_port) {
		m_id = id;
		m_node = node;
		m_socket = Socket::CreateSocket (node, UdpSocketFactory::GetTypeId ());
		m_local_address = local_address;
		m_local_port = local_port;
		m_remote_address = remote_address;
		m_remote_port = remote_port;
		m_timer = timer;

		uint8_t idx_np = rand() % ddos_traffic.n_packets.size();
		uint8_t idx_ps = rand() % ddos_traffic.packet_size.size();
		m_nPackets = ddos_traffic.n_packets[idx_np];
		m_packetSize = ddos_traffic.packet_size[idx_ps];

		botnet_timer_setup();
		for (uint8_t i = 0; i < number_of_iot; i++)
			is_attack[i] = 0;

		// SetStartTime(MilliSeconds(0));
		std::cout << "[Bot " << m_id << "]  Setup" << std::endl;
		}

void BotApplication::StartApplication (void)
{	
	NS_LOG_FUNCTION_NOARGS ();

	Inet6SocketAddress local = Inet6SocketAddress (m_local_address, m_local_port);
	m_socket->Bind (local);

	m_socket->SetRecvCallback (MakeCallback (&BotApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback (&BotApplication::HandleSend, this));
		

	std::cout << "[Bot " << m_id << "]  Start Application" << std::endl;
	Schedule();
}

void BotApplication::Schedule() {
	// std::cout << "Sim: " << Simulator::Now() << " Start: " << timer->start << " End: " << timer->finish << std::endl;
	if (timer->finish >= Simulator::Now() && timer->start <= Simulator::Now()) {
		// if (is_start) {
		// 	std::cout << "Bot " << m_id << " Application Start" << std::endl;
		// }
		is_start = false;
		timer->trigger = false;
		for (uint8_t i = 0; i < number_of_iot; i++)
			is_attack[i] = 1;
		Send();
		Simulator::Schedule (timer->interval, &BotApplication::Schedule, this);
	}
	else {
		is_start = true;
		botnet_timer_setup();
		// std::cout << "Bot " << m_id << " Application Pause [ " << timer->start.GetMilliSeconds() << " millisec ]" << std::endl;
		Simulator::Schedule (timer->start - Simulator::Now(), &BotApplication::Schedule, this);
	}
}

void BotApplication::Send() {
	NS_LOG_FUNCTION (this);
 
	uint8_t* buffer = new uint8_t[m_packetSize];
	for (uint16_t i = 0; i < m_packetSize; i++){
		buffer[i] = (rand() % 0xFF) + 0x04;
	}

	for (uint i = 0; i < m_remote_address.size(); i++) {
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
		// std::cout << "Bot " << m_id << " -> IOT " << i << " [ delta = " 
		// 	<< timer->interval.GetMilliSeconds() << " millisec ]" << std::endl; 
	}
}

void BotApplication::StopApplication ()
{
	is_attack = 0;
	NS_LOG_FUNCTION_NOARGS ();

	std::cout << "[Bot " << m_id << "] Stop Application" << std::endl;
	m_socket->Close();
}

void BotApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {}

void BotApplication::HandleRead (Ptr<Socket> socket) {}

NS_OBJECT_ENSURE_REGISTERED (IOTApplication);
