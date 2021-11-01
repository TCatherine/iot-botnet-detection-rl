// #include "sim.h"


// void simulation_NetAnim_configuration(
//     NodeContainer& iotNodes,
//     NodeContainer& botNodes,
//     NodeContainer& routerNode,
//     NodeContainer& clientNodes){
//     //Simulation NetAnim configuration and node placement
//     MobilityHelper mobility;

//     mobility.SetPositionAllocator("ns3::GridPositionAllocator",
//                                   "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0),
//                                   "GridWidth", UintegerValue(5), "LayoutType", StringValue("RowFirst"));

//     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

//     mobility.Install(routerNode);
//     mobility.Install(botNodes);
//     mobility.Install(clientNodes);
//     mobility.Install(iotNodes);

//     AnimationInterface anim("DDoSim.xml");


//     // ns3::AnimationInterface::SetConstantPosition(routerNode.Get(0), 20, 20);
//     // uint32_t x_pos = 0;
//     // for (int l = 0; l < NUMBER_OF_BOTS; ++l)
//     // {
//     //     ns3::AnimationInterface::SetConstantPosition(botNodes.Get(l), x_pos+=2, 30);
//     // }

//     // for (int l = 0; l < NUMBER_OF_IOT; ++l)
//     // {
//     //     ns3::AnimationInterface::SetConstantPosition(iotNodes.Get(l), 30+2*l, 20);
//     // }

//     // for (int l = 0; l < NUMBER_OF_CLIENTS; ++l)
//     // {
//     //     ns3::AnimationInterface::SetConstantPosition(clientNodes.Get(l), 0, 2*l);
//     // }
// }