/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#include "env.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/node-list.h"
#include "ns3/log.h"
#include <sstream>
#include <iostream>

int16_t reward_tp, reward_fp, reward_tn, reward_fn;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IotEnv");

NS_OBJECT_ENSURE_REGISTERED (IotEnv);

IotEnv::IotEnv ()
{
  NS_LOG_FUNCTION (this);
  m_interval = Seconds(20);
  cur_reward = 0;
  fp = 0;
  fn = 0;
  tp = 0;
  tn = 0;
  Simulator::Schedule (Seconds(0.0), &IotEnv::ScheduleNextStateRead, this);
}

IotEnv::IotEnv (uint32_t agentId, Time stepTime)
{
  NS_LOG_FUNCTION (this);
  m_agentId = agentId;
  m_interval = stepTime;
  cur_reward = 0;
  fp = 0;
  fn = 0;
  tp = 0;
  tn = 0;

  Simulator::Schedule (Seconds(0.0), &IotEnv::ScheduleNextStateRead, this);
}

void
IotEnv::ScheduleNextStateRead ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Schedule (m_interval, &IotEnv::ScheduleNextStateRead, this);
  Notify();
}

IotEnv::~IotEnv ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
IotEnv::GetTypeId (void)
{
  static TypeId tid = TypeId ("IotEnv")
    .SetParent<OpenGymEnv> ()
    .SetGroupName ("OpenGym")
    .AddConstructor<IotEnv> ()
  ;
  return tid;
}

void
IotEnv::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

/*Define observation space*/
Ptr<OpenGymSpace> IotEnv::GetObservationSpace()
{
  uint32_t nodeNum = 9;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();

  Ptr<OpenGymDiscreteSpace> discrete = CreateObject<OpenGymDiscreteSpace> (nodeNum);
  Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);

  Ptr<OpenGymDictSpace> space = CreateObject<OpenGymDictSpace> ();
  space->Add("box", box);
  space->Add("discrete", discrete);

  // NS_LOG_UNCOND ("AgentID: " << m_agentId << " MyGetObservationSpace: " << space);
 return space;
}

/*Collect observations*/
Ptr<OpenGymDataContainer> IotEnv::GetObservation()
{
  uint32_t parameterNum = 10;
  uint32_t number_packet = vector_features.size();
  std::vector<uint32_t> shape = {number_packet,parameterNum};
  Ptr<OpenGymBoxContainer<int64_t>> box = CreateObject<OpenGymBoxContainer<int64_t>>(shape);

  for (uint32_t i = 0; i< vector_features.size(); i++){
    box->AddValue(Simulator::Now().GetMicroSeconds ());
    //box->AddValue(DynamicCast<uint32_t>(m_sendEvent));
    box->AddValue(vector_features[i].size);
    box->AddValue(vector_features[i].size);
    box->AddValue(vector_features[i].uid);
    box->AddValue(vector_features[i].sourcePort);
    box->AddValue(vector_features[i].destinationPort);
    box->AddValue(vector_features[i].length);
    box->AddValue(vector_features[i].flags);
    box->AddValue(vector_features[i].windowSize);
    box->AddValue(vector_features[i].urgentPointer);
  }
  // Print data
  //NS_LOG_INFO ("MyGetObservation: " << box);
  return box;
}

/*Define action space*/
Ptr<OpenGymSpace> IotEnv::GetActionSpace()
{
  uint32_t parameterNum = 2;
  float low = -4.611686e+18;
  float high = 4.611686e+18;
  std::vector<uint32_t> shape = {parameterNum,};
  std::string dtype = TypeNameGet<int64_t> ();

  Ptr<OpenGymBoxSpace> box = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_INFO ("MyGetActionSpace: " << box);
  return box;
}

/*Define game over condition*/
bool IotEnv::GetGameOver()
{
  bool isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == MAX_STEP && test) {
      isGameOver = true;
  }
  return isGameOver;
}

uint16_t IotEnv::Epsilon_Greedy(std::vector<int> action){
  float j = 0;
  float a = 100.0;
  eps = 0.05;
  float p = (float(rand())/float((RAND_MAX)) * a);
    if (p < eps)
      j = rand() % 2 ;
    else if (action[0] > action[1])
      j = 0;
    else j = 1;
  a += 100.0;
//j = std::max_element(action.begin(), action.end());
  return j;
}

/*Define reward function*/
float IotEnv::GetReward()
{
  float reward = cur_reward;
  //std::cout << "Reward " << reward << " " << cur_reward << std::endl;
  cur_reward=0;
  return reward;
}

/*Define extra info. Optional*/
std::string IotEnv::GetExtraInfo()
{
  std::string info = "FP " + std::to_string(fp);
  info +=  " FN " + std::to_string(fn);
  info +=  " TP " + std::to_string(tp);
  info +=  " TN " + std::to_string(tn);
  //std::string info = "Active Agent " + std::to_string(m_agentId);
  return info;
}

/*Execute received actions*/
bool IotEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymBoxContainer<int32_t> > box = DynamicCast<OpenGymBoxContainer<int32_t>>(action);
  std::vector<int> list_action;

  list_action.push_back(box->GetValue(0));
  list_action.push_back(box->GetValue(1));

  int idx_action = Epsilon_Greedy(list_action);
  fp = 0;
  fn = 0;
  tp = 0;
  tn = 0;
  
  if (is_attack) {
    if (idx_action == is_attack){
      tn = 1;
      cur_reward = reward_tn;
    }
    else {
      cur_reward = reward_fp;
      fp = 1;
    }
  }
  else if (idx_action == is_attack) {
      cur_reward = reward_tp;
      tp = 1;
  }
      else 
      {
        cur_reward = reward_fn;
        fn = 1;
      }


  //std::string name_action[] = {"Clear Traffic", "Botnet Detect"};
  //NS_LOG_UNCOND ("Agent " << m_agentId << "\t Prediction: " << name_action[idx_action] << "\tTrue: " << name_action[is_attack] << "\t Reward: " << cur_reward);


  return true;
}

} // ns3 namespace