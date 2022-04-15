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
#include "env.h"

using namespace ns3;
using namespace std;

bool* is_attack = nullptr;

NS_LOG_COMPONENT_DEFINE ("Server_App");

Ipv6Address CreateStackProtocol(Ptr<Node> node, Ipv6AddressHelper& ipv6, Ptr<Node> router, bool is_iot = false) {
	CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  	csma.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  	csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	if (is_tracing && is_iot)
		csma.EnablePcapAll("pcap");

	Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
	SixLowPanHelper sixlowpan;
	sixlowpan.SetDeviceAttribute ("ForceEtherType", BooleanValue (true) );

	node->AddDevice(dev0);

	NodeContainer iot_router_container = NodeContainer(node, router);
	NetDeviceContainer temp_container = csma.Install(iot_router_container);

	 
	NetDeviceContainer net_device_container = sixlowpan.Install(temp_container);
	

	Ipv6InterfaceContainer ipv6_int_container = ipv6.Assign(net_device_container);
	ipv6_int_container.SetForwarding(1, true);
	ipv6_int_container.SetDefaultRouteInAllNodes(1);

	return ipv6_int_container.GetAddress(0, 1);
}

int  main (int argc, char *argv[])
{
	parse();
	is_attack = new bool[number_of_iot];
	// botnet_timer_setup();

	GlobalValue::Bind ("SimulatorImplementationType",
	StringValue ("ns3::RealtimeSimulatorImpl"));

	LogComponentEnable("Server_App", LOG_LEVEL_INFO);
	LogComponentEnable("Client_App", LOG_LEVEL_INFO);
	LogComponentEnable("IOT_App", LOG_LEVEL_INFO);
    LogComponentEnable("Botnet_App", LOG_LEVEL_INFO);

  	NS_LOG_INFO ("Creating nodes");
	NodeContainer iotNodes, routerNode, botNodes, clientNodes;

	clientNodes.Create(number_of_clients);
	routerNode.Create(1);
    iotNodes.Create(number_of_iot);
    botNodes.Create(number_of_bots);

	NS_LOG_INFO ("Installing Internet stack on nodes");
    InternetStackHelper stack;
    stack.Install(iotNodes);
    stack.Install(routerNode);
    stack.Install(clientNodes);
    stack.Install(botNodes);

	NS_LOG_INFO ("Ipv6AddressHelper init");
	Ipv6AddressHelper iot_ipv6;
	Ipv6AddressHelper external_ipv6;
	iot_ipv6.SetBase(Ipv6Address ("2001:1::"), Ipv6Prefix (64));
	external_ipv6.SetBase(Ipv6Address ("2001:2::"), Ipv6Prefix (64));

	std::vector<Ipv6Address> addresses;
	std::vector<uint16_t> ports;

	NS_LOG_INFO ("Create Stack Protocol for IOT");
	for (uint16_t i = 0; i < number_of_iot; i++, iot_ipv6.NewNetwork()) {
		Ipv6Address addr = CreateStackProtocol(iotNodes.Get (i), iot_ipv6, routerNode.Get(0), true);

		addresses.push_back(addr);
		ports.push_back(TCP_SINK_PORT);

		Ptr<IOTApplication> iot_app = CreateObject<IOTApplication> ();
		iot_app->Setup (i, iotNodes.Get (i), addr, TCP_SINK_PORT,  1040, 1000, DataRate ("1Mbps"));

		iotNodes.Get (i)->AddApplication (iot_app);
		iot_app->SetStartTime (Seconds (1.));
	}

	NS_LOG_INFO ("Create Stack Protocol for Clients");
	for (uint16_t i = 0; i < number_of_clients; i++, external_ipv6.NewNetwork ()) {
		Ipv6Address addr = CreateStackProtocol(clientNodes.Get(i), external_ipv6, routerNode.Get(0));

		Ptr<ClientApplication> app = CreateObject<ClientApplication> ();
		app->Setup (i, clientNodes.Get (i), addr, 
					TCP_SINK_PORT, addresses, ports);

		clientNodes.Get (i)->AddApplication (app);
	}

	NS_LOG_INFO ("Create Stack Protocol for Bots");
	for (uint16_t i = 0; i < number_of_bots; i++, external_ipv6.NewNetwork ()) {
		Ipv6Address addr = CreateStackProtocol(botNodes.Get(i), external_ipv6, routerNode.Get(0));

		Ptr<BotApplication> app = CreateObject<BotApplication> ();
		app->Setup (i, botNodes.Get (i), addr, TCP_SINK_PORT, addresses, ports);
		botNodes.Get (i)->AddApplication (app);
	}

	Animation(std::ref(routerNode), std::ref(botNodes), std::ref(iotNodes), std::ref(clientNodes));

	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}
