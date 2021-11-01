#include "sim.h"

NS_LOG_COMPONENT_DEFINE ("IOT_App");

IOTApplication::IOTApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	m_node = 0;
	m_running  = false;
	m_packetSize = 0;
	m_env_port = 0;
	m_env = 0;
};

IOTApplication::~IOTApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void IOTApplication::Setup (uint16_t id, Ptr<Socket> socket, Ipv6Address local_address, 
	uint16_t local_port, uint32_t packetSize, uint32_t nPackets, DataRate dataRate){
		m_id = id;
		m_socket = socket;
		m_address = local_address;
		m_port = local_port;
		m_packetSize = packetSize;
		m_nPackets = nPackets;
		m_dataRate = dataRate;
		m_env_port = 5555 + id;
		std::cout << "[IOT "<< m_id << "] Setup: " << m_address <<  ":" << m_port << std::endl;
}

void IOTApplication::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void IOTApplication::StartApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	
	m_running = true;
							
	Inet6SocketAddress local = Inet6SocketAddress (m_address, m_port);
	m_socket->Bind (local);
	m_socket->Listen();
	m_socket->SetRecvCallback (MakeCallback (&IOTApplication::HandleRead, this));
	m_socket->SetSendCallback (MakeCallback(&IOTApplication::HandleSend, this));

	m_env = CreateObject<IotEnv> (m_id, Seconds(13));
	Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface> (m_env_port);
	m_env->SetOpenGymInterface(openGymInterface);
}

void IOTApplication::StopApplication ()
{  
	NS_LOG_FUNCTION_NOARGS ();
	m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
	  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  m_socket = 0;
    }

	Simulator::Cancel (m_env->m_sendEvent);
}

void IOTApplication::HandleSend (Ptr<Socket> socket, uint32_t available) {

	//std::cout << "[IOT] Handle Send " << std::endl;
}

void IOTApplication::HandleRead (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	//std::cout << "[IOT] Handle Read" << std::endl;
	while (packet = socket->RecvFrom (from))
	{
		TcpHeader tcp_header;
		packet->PeekHeader (tcp_header);
		struct features f;
		f.sourcePort = tcp_header.GetSourcePort();
		f.destinationPort = tcp_header.GetDestinationPort();
		f.sequenceNumber = tcp_header.GetSequenceNumber();
		f.ackNumber = tcp_header.GetAckNumber();
		f.length = tcp_header.GetLength();
		f.flags = tcp_header.GetFlags();
		f.windowSize = tcp_header.GetWindowSize();
		f.urgentPointer = tcp_header.GetUrgentPointer();
		f.size = packet->GetSize();
		f.uid = packet->GetUid();
		//PacketTagIterator tags = packet->GetPacketTagIterator();
		//packet->PrintByteTags(std::cout);
		m_env->vector_features.push_back(f);
		//packet->CopyData (buffer, 1024); 
		// NS_LOG_INFO ( "[IOT "<< m_id << "] Received " << packet->GetSize () << " bytes from client " 
		// << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ());


		uint8_t *buffer = new uint8_t [1024]; 
		Inet6SocketAddress remote = Inet6SocketAddress::ConvertFrom (from);
		Ptr<Packet> p;
		buffer=(uint8_t *)"AAAAAAA";
		p = new Packet(buffer, 1024);
		m_socket->SendTo(p, 0, remote);
	}
}