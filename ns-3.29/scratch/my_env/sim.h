#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <ns3/csma-helper.h>
#include "ns3/mobility-module.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/node-container.h"
#include "ns3/sixlowpan-module.h"
#include <ns3/lr-wpan-module.h>
#include "env.h"

using namespace ns3;

#define TCP_SINK_PORT 10
#define UDP_SINK_PORT 9001

struct config {
	std::vector <std::string> data_rate;
	std::vector <std::string> delay;
	std::vector <uint32_t> packet_size;
	std::vector <uint16_t> n_packets;
};

struct packets_flow {
	Ipv6Address addrs;
	uint32_t port;
	uint16_t packet_number;
	uint16_t sum_size_packet;
	uint8_t sum_ttl;
	Time s_time;
	Time l_time;
	Time duration;
	Time sum_interval;
	Time l_interval;
	Time max_deviation_interval;
};

struct botnet_timer {
		Time			interval;
		Time 			start;
		Time 			finish; 
		bool			trigger;
};


extern botnet_timer* timer;
extern void botnet_timer_setup();

extern config clear_traffic;
extern config ddos_traffic;

//Number of Entities
extern uint16_t number_of_bots, number_of_clients, number_of_iot;
extern int16_t reward_tp, reward_fp, reward_tn, reward_fn;
extern uint32_t botnet_attack_start;
extern uint32_t botnet_attack_finish;

void parse();
void GenerateInterval();
uint32_t GetTime(Time time);

void Animation (
	NodeContainer& routerNode, NodeContainer& botNodes, 
	NodeContainer& iotNodes,NodeContainer& clientNodes);


class IOTApplication : public IApplicationBase, public Application
{
public:
	IOTApplication();
	virtual ~IOTApplication ();
	void Setup (uint16_t id, Ptr<Node> node, Ipv6Address local_address, 
	uint16_t local_port, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

	Ptr<OpenGymInterface> env_interface;
	virtual void PacketFlowConvert() override;
	virtual void PacketFlowClear() override;

protected:
	using flow_it = std::vector<struct packets_flow>::iterator;
	virtual void DoDispose (void);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void HandleRead (Ptr<Socket> socket);
	void Accept (Ptr< Socket > socket, const Address& from);
	void HandleSend (Ptr<Socket> socket, uint32_t available);
	void Create (Ptr< Socket > socket);
	void PacketParse(Ptr<Packet> packet);

	void ScheduleTx (void);
	void SendPacket (void);

	uint16_t		m_id;
	Ptr<Socket>		m_socket;
	Ptr <Node> 		m_node; 
//Address         m_peer;
	Ipv6Address 	m_address;
	uint16_t		m_port;
	uint32_t        m_packetSize;
	uint32_t        m_nPackets;
	DataRate        m_dataRate;
	uint32_t        m_packetsSent;
	uint16_t 		m_env_port;

	Ptr<IotEnv> 	m_env;	
	std::vector<packets_flow> vector_packet_flow;	
};

class BotApplication : public Application 
{
	public:
		BotApplication();
		void Setup (uint16_t id, Ptr<Node> socket, Ipv6Address local_address, uint16_t local_port, 
			std::vector<Ipv6Address>& remote_address, std::vector<uint16_t>& remote_port);
		
		//BotApplication (Ptr<Node> node, Ipv4Address address, uint16_t port);
		virtual ~BotApplication ();

	protected:
		virtual void DoDispose (void);

	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void HandleRead (Ptr<Socket> socket);
		void HandleSend (Ptr<Socket> socket, uint32_t available);
		void NextSend ();
		void Schedule();
		void Send();

		uint16_t		m_id;
		Ptr <Socket>    m_socket;
		Ptr <Node> 		m_node; 
		Ptr <Node> 		server_node;
	//Address         m_peer;
		Ipv6Address 	m_local_address;
		uint16_t		m_local_port;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		uint32_t        m_packetsSent;
		uint16_t 		m_env_port;

		botnet_timer* 				m_timer;
		std::vector<Ipv6Address>	m_remote_address;
		std::vector<uint16_t>		m_remote_port;
};

class ClientApplication : public Application 
{
	public:
		ClientApplication();
		void Setup (uint16_t id, Ptr<Node> node, Ipv6Address local_address, uint16_t local_port, 
			std::vector<Ipv6Address>& remote_address, std::vector<uint16_t>& remote_port);
		
		virtual ~ClientApplication ();
	protected:
		virtual void DoDispose (void);

	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void HandleRead (Ptr<Socket> socket);
		void HandleSend (Ptr<Socket> socket, uint32_t available);
		void NextSend ();
		void Send();
		void Schedule();

		uint16_t		m_id;
		Ptr <Socket>    m_socket;
		Ptr <Node> 		m_node; 
		Ptr <Node> 		server_node;
		Ipv6Address		m_local_address;
		uint16_t		m_local_port;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		uint32_t        m_packetsSent;
		uint16_t 		m_env_port;
		Time			m_interval;
		Time 			time_start;
		Time 			time_finish; 

		std::vector<Ipv6Address>	m_remote_address;
		std::vector<uint16_t>		m_remote_port;
};