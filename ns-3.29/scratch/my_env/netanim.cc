#include "sim.h"


void Animation (
	NodeContainer& routerNode, NodeContainer& botNodes, 
	NodeContainer& iotNodes,NodeContainer& clientNodes) {
		static MobilityHelper mobility;

		mobility.SetPositionAllocator("ns3::GridPositionAllocator",
									"MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), 
									"DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0),
									"GridWidth", UintegerValue(5), "LayoutType", StringValue("RowFirst"));

		mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

		mobility.Install(routerNode);
		mobility.Install(botNodes);
		mobility.Install(clientNodes);
		mobility.Install(iotNodes);

		static AnimationInterface anim("DDoSim.xml");

		const std::string sourceDir = "./icons";
		
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
	
		anim.EnablePacketMetadata (true);

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
}