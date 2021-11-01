/*       
 * 
 * In this we follow the following setup / node placement
 * 
 *(c1),(c2)...(cn)               (iot1)
 *  \  /     /                  /-(iot2)
 *   \/     /                  /  ...
 *    ------------- (n2) ------- (iotn)
 *                 / |  \      
 *                /  |   \      
 *               /   |    \
 *            (B1),(B2)...(Bn) 
 *                 
 *  c1, c2, c3 is legitimate users, communicating with iots
 *  B0-Bn are bots DDoS-ing the network.
 * 
 *	Protocol Stack      
 * NB IoT based on LTE                 
 * 		+---------+                          
 *  	| UDP     |    
 * 		+---------+  
 *  	| IPv6    |   
 *  	+---------+ 
 *  	| 6LoWPAN |  
 *  	+---------+  
 *  	| CSMA    |   
 *  	+---------+   
 * 
 * NetAnim XML is saved as -> DDoSim.xml 
 *  
 */

#include "sim.h"

using namespace ns3;
using namespace std;

int is_attack;

NS_LOG_COMPONENT_DEFINE ("Server_App");

void GenerateInterval(){
	Time cur_time = Simulator::Now();
	botnet_attack_start = rand() % 500 + 1;
	botnet_attack_finish = rand() % 1000 + botnet_attack_start;
	Simulator::Schedule (Seconds(botnet_attack_finish) + cur_time, GenerateInterval);
}

int  main (int argc, char *argv[])
{
	parse();
  	is_attack = 0;
	LogComponentEnable("Server_App", LOG_LEVEL_INFO);
	LogComponentEnable("Client_App", LOG_LEVEL_INFO);
	LogComponentEnable("IOT_App", LOG_LEVEL_INFO);
    LogComponentEnable("Botnet_App", LOG_LEVEL_INFO);

	NS_LOG_INFO ("Creating nodes");
	NodeContainer iotNodes;
	NodeContainer routerNode;
	NodeContainer botNodes;
	NodeContainer clientNodes;

	clientNodes.Create(number_of_clients);
	routerNode.Create(1);
    iotNodes.Create(number_of_iot);
    botNodes.Create(number_of_bots);
	NS_LOG_INFO ("Finished creating nodes");

	Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();

	PointToPointHelper p2p_iot, p2p_client, p2p_bot;
	SixLowPanHelper sixlowpan_iot, sixlowpan_client, sixlowpan_bot, sixlowpan_router;

	 CsmaHelper csma;
   csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
   csma.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
   csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
   
	std::vector<NetDeviceContainer> device_iot, device_client, device_bot;

	NS_LOG_INFO ("Set IOT Device");
	std::vector <NodeContainer> connect_iot, connect_bot, connect_client;

	for (uint16_t i = 0; i < number_of_iot; i++) {	
		connect_iot.push_back(NodeContainer(iotNodes.Get(i), routerNode.Get(0)));
		iotNodes.Get(i)->AddDevice(dev0);
		uint16_t rate_idx = rand() % clear_traffic.data_rate.size();
		uint16_t delay_idx = rand() % clear_traffic.delay.size();

		p2p_iot.SetDeviceAttribute ("DataRate", StringValue (clear_traffic.data_rate[rate_idx]));
		p2p_iot.SetChannelAttribute ("Delay", StringValue (clear_traffic.delay[delay_idx]));
		sixlowpan_iot.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );

		NetDeviceContainer temp_container = csma.Install(connect_iot[connect_iot.size()-1]);
		device_iot.push_back(sixlowpan_iot.Install(temp_container));
		//device_iot.push_back(sixlowpan_iot.Install(temp_container));
		//device_iot.push_back(temp_container);
	}

	NS_LOG_INFO ("Set Client Device");

	for (uint16_t i = 0; i < number_of_clients; i++) {
		connect_client.push_back(NodeContainer(clientNodes.Get(i), routerNode.Get(0)));
		clientNodes.Get(i)->AddDevice(dev0);
		uint16_t rate_idx = rand() % clear_traffic.data_rate.size();
		uint16_t delay_idx = rand() % clear_traffic.delay.size();
		p2p_client.SetDeviceAttribute ("DataRate", StringValue (clear_traffic.data_rate[rate_idx]));
		p2p_client.SetChannelAttribute ("Delay", StringValue (clear_traffic.delay[delay_idx]));
		sixlowpan_client.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );

		//NetDeviceContainer temp_container = p2p_client.Install (clientNodes.Get(i), routerNode.Get(0));
		//device_client.push_back(sixlowpan_client.Install(NetDeviceContainer(clientNodes.Get(i), routerNode.Get(0)));
		NetDeviceContainer temp_container = csma.Install(connect_client[connect_client.size()-1]);
		device_client.push_back(sixlowpan_client.Install(temp_container));
	}

	NS_LOG_INFO ("Set Botnet Device");

	for (uint16_t i = 0; i < number_of_bots; i++) {
		connect_bot.push_back(NodeContainer(botNodes.Get(i), routerNode.Get(0)));
		botNodes.Get(i)->AddDevice(dev0);
		uint16_t rate_idx = rand() % ddos_traffic.data_rate.size();
		uint16_t delay_idx = rand() % ddos_traffic.delay.size();
		p2p_bot.SetDeviceAttribute ("DataRate", StringValue (ddos_traffic.data_rate[rate_idx]));
		p2p_bot.SetChannelAttribute ("Delay", StringValue (ddos_traffic.delay[delay_idx]));
		sixlowpan_bot.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );

		//NetDeviceContainer temp_container = p2p_bot.Install (botNodes.Get(i), routerNode.Get(0));
		//device_bot.push_back(sixlowpan_bot.Install(NetDeviceContainer(botNodes.Get(i), routerNode.Get(0)));
		NetDeviceContainer temp_container = csma.Install(connect_bot[connect_bot.size()-1]);
		device_bot.push_back(sixlowpan_bot.Install(temp_container));
	}
  
	NS_LOG_INFO ("Installing Internet stack on nodes");
    InternetStackHelper stack;
    stack.Install(iotNodes);
    stack.Install(routerNode);
    stack.Install(clientNodes);
    stack.Install(botNodes);
	NS_LOG_INFO ("Finished installing internet stack");

	NS_LOG_INFO ("Started creating connections");
	Ipv6AddressHelper iot_ipv6;
	Ipv6AddressHelper external_ipv6;
	iot_ipv6.SetBase(Ipv6Address ("2001:1::"), Ipv6Prefix (64));
	external_ipv6.SetBase(Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  
	std::vector<Ipv6InterfaceContainer> iot_int;
	std::vector<Ipv6InterfaceContainer> router_int;
	std::vector<Ipv6InterfaceContainer> client_ip_interface;
	std::vector<Ipv6InterfaceContainer> bot_ip_interface;

	std::vector<Ipv6Address> addresses;
	std::vector<uint16_t> ports;
	NS_LOG_INFO ("Started creating IOT connections");
	for (uint16_t i = 0; i < number_of_iot; i++){
		iot_int.push_back(iot_ipv6.Assign(device_iot[i]));
		iot_int[iot_int.size()-1].SetForwarding(1, true);
		iot_int[iot_int.size()-1].SetDefaultRouteInAllNodes(1);
		//router_int.push_back(iot_ipv6.Assign(device_router));
		iot_ipv6.NewNetwork();
		addresses.push_back(iot_int[i].GetAddress(0, 1));
		ports.push_back(TCP_SINK_PORT);
	}

	NS_LOG_INFO ("Started creating Client connections");
	for (uint16_t i = 0; i < number_of_clients; i++){
		client_ip_interface.push_back(external_ipv6.Assign(device_client[i]));
		client_ip_interface[client_ip_interface.size()-1].SetForwarding(1, true);
		client_ip_interface[client_ip_interface.size()-1].SetDefaultRouteInAllNodes(1);
		//router_int.push_back(iot_ipv6.Assign(device_router));
		external_ipv6.NewNetwork ();
	}

	NS_LOG_INFO ("Started creating Bot connections");
	for (uint16_t i = 0; i < number_of_bots; i++) {
		bot_ip_interface.push_back(external_ipv6.Assign(device_bot[i]));
		bot_ip_interface[bot_ip_interface.size()-1].SetForwarding(1, true);
		bot_ip_interface[bot_ip_interface.size()-1].SetDefaultRouteInAllNodes(1);
		//router_int.push_back(iot_ipv6.Assign(device_router));
		external_ipv6.NewNetwork ();
	}
	NS_LOG_INFO ("Finished creating connections");

	// NS_LOG_INFO ("Populating routing tables");
 	// Ipv6GlobalRoutingHelper::PopulateRoutingTables ();
 	// NS_LOG_INFO ("Finished populating routing tables");

	NS_LOG_INFO ("Installing IOT Applications");
	for (uint16_t i = 0; i < number_of_iot; i++ ){
		Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (iotNodes.Get (i), UdpSocketFactory::GetTypeId ());
		Ptr<IOTApplication> iot_app = CreateObject<IOTApplication> ();
		iot_app->Setup (i, ns3TcpSocket, iot_int[i].GetAddress(0, 1), TCP_SINK_PORT,  1040, 1000, DataRate ("1Mbps"));
		iotNodes.Get (i)->AddApplication (iot_app);
		iot_app->SetStartTime (Seconds (1.));
		//iot_app->SetStopTime (Seconds (100.));
	}
	NS_LOG_INFO ("Finished installing IOT Applications");

	NS_LOG_INFO ("Installing Client Applications");
	for (uint16_t i = 0; i < number_of_clients; i++ ){
		Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (clientNodes.Get (i), UdpSocketFactory::GetTypeId ());
		Ptr<ClientApplication> app = CreateObject<ClientApplication> ();
		app->Setup (i, ns3TcpSocket, client_ip_interface[i].GetAddress(0, 1), 
					TCP_SINK_PORT, addresses, ports);

		clientNodes.Get (i)->AddApplication (app);
	}
	NS_LOG_INFO ("Finished installing Client Applications");


	NS_LOG_INFO ("Installing Botnet Applications");
	GenerateInterval();
	
	for (uint16_t i = 0; i < number_of_bots; i++ ){
		Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (botNodes.Get (i), UdpSocketFactory::GetTypeId ());
		Ptr<BotApplication> app = CreateObject<BotApplication> ();
		app->Setup (i, ns3TcpSocket, bot_ip_interface[i].GetAddress(0, 1), TCP_SINK_PORT, addresses, ports);
		botNodes.Get (i)->AddApplication (app);
	}
	NS_LOG_INFO ("Finished installing Botnet Applications");

	MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(5), "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(routerNode);
    mobility.Install(botNodes);
    mobility.Install(clientNodes);
    mobility.Install(iotNodes);

    AnimationInterface anim("DDoSim.xml");

	string sourceDir = "./icons";
	 
	uint32_t pictureIoT = anim.AddResource(sourceDir + "/iot.png");
    //uint32_t pictureGate = anim.AddResource(sourceDir + "/gate.png");
    uint32_t pictureRouter = anim.AddResource(sourceDir + "/router.png");
    uint32_t pictureClient = anim.AddResource(sourceDir + "/client.png");
	uint32_t pictureBot = anim.AddResource(sourceDir + "/attacker.png");

	uint16_t idx=0;
	for (uint16_t i = 0; i < number_of_clients; i++, idx++){
		anim.UpdateNodeImage(idx, pictureClient);
		anim.UpdateNodeSize(idx, 3, 3);
	}

	anim.UpdateNodeSize(idx, 5, 5);
    anim.UpdateNodeImage(idx++, pictureRouter);

	for (uint16_t i = 0; i < number_of_iot; i++, idx++){
			anim.UpdateNodeImage(idx, pictureIoT);
			anim.UpdateNodeSize(idx, 3, 3);
	}

	for (uint16_t i = 0; i < number_of_bots; i++, idx++){
			anim.UpdateNodeImage(idx, pictureBot);
			anim.UpdateNodeSize(idx, 3, 3);
	}
    
 
	//anim.EnablePacketMetadata (true);

    ns3::AnimationInterface::SetConstantPosition(routerNode.Get(0), 20, 10);
	//anim.UpdateNodeImage(0, pictureGate);
    uint32_t x_pos = 0;
    for (int l = 0; l < number_of_bots; ++l){
        ns3::AnimationInterface::SetConstantPosition(botNodes.Get(l), x_pos+=3, 30);
    }
    for (int l = 0; l < number_of_iot; ++l){
        ns3::AnimationInterface::SetConstantPosition(iotNodes.Get(l), 30+3*l, 10+3*l);
    }
    for (int l = 0; l < number_of_clients; ++l) {
        ns3::AnimationInterface::SetConstantPosition(clientNodes.Get(l), 0, 3*l);
    }

	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}