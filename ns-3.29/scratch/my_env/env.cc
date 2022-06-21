/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/opengym-module.h"
#include <cstdint>

#include "env.h"

int16_t reward_tp, reward_fp, reward_tn, reward_fn;
int num_env = 0;

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
    dataset_file.open ("example.csv");
    dataset_file << "№,pack_num,pack_size,duration,interval,deviation_interval,is_attack" << std::endl;
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
    std::string name = "dataset" + std::to_string(num_env) + '_' + std::to_string(m_agentId) + ".csv";
    dataset_file.open (name);
    dataset_file << "№,pack_num,pack_size,duration,interval,deviation_interval,is_attack" << std::endl;

    Simulator::Schedule (Seconds(0.0), &IotEnv::ScheduleNextStateRead, this);
  }

  void
  IotEnv::ScheduleNextStateRead ()
  {
    NS_LOG_FUNCTION (this);
    zmq_sleep(1);
    Simulator::Schedule (m_interval, &IotEnv::ScheduleNextStateRead, this);
    Notify();
  }

  IotEnv::~IotEnv ()
  {
    NS_LOG_FUNCTION (this);
    dataset_file.close();
  }

  TypeId IotEnv::GetTypeId (void) {
    static TypeId tid = TypeId ("IotEnv")
      .SetParent<OpenGymEnv> ()
      .SetGroupName ("OpenGym")
      .AddConstructor<IotEnv> ();
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
    uint32_t nodeNum = 6;
    uint16_t low = 0.0;
    uint16_t high = std::numeric_limits<uint16_t>::max();
    std::vector<uint32_t> shape = {nodeNum,};
    std::string dtype = TypeNameGet<uint16_t> ();

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
    m_application->PacketFlowConvert();
    uint32_t parameterNum = 6;
    uint32_t number_packet = vector_features.size();
    std::vector<uint32_t> shape = {number_packet,parameterNum};
    Ptr<OpenGymBoxContainer<int64_t>> box = CreateObject<OpenGymBoxContainer<int64_t>>(shape);

    for (uint32_t i = 0; i < vector_features.size(); i++){
      //box->AddValue(DynamicCast<uint32_t>(m_sendEvent));
      // for (uint8_t j = 0; j < 16; j++)
      //   box->AddValue(vector_features[i].addrs[j]);
      box->AddValue(i);
      // box->AddValue(vector_features[i].port);
      box->AddValue(vector_features[i].packet_number);
      box->AddValue(vector_features[i].average_size_packet);
      // box->AddValue(vector_features[i].average_ttl);
      // box->AddValue(vector_features[i].s_time);
      // box->AddValue(vector_features[i].l_time);
      box->AddValue(vector_features[i].duration);
      box->AddValue(vector_features[i].average_interval);
      box->AddValue(vector_features[i].max_deviation_interval);

      bool is_att = is_attack[m_agentId] || m_is_attack ? 1: 0;

      dataset_file << i << "," << vector_features[i].packet_number << ',' << vector_features[i].average_size_packet <<
      ',' << vector_features[i].duration << ',' <<  vector_features[i].average_interval <<
      ',' << vector_features[i].max_deviation_interval << ',' << is_att << std::endl;
    }
    // // Print data
    // //NS_LOG_INFO ("MyGetObservation: " << box);
    return box;
  }

  /*Define action space*/
  Ptr<OpenGymSpace> IotEnv::GetActionSpace()
  {
    uint16_t parameterNum = 2;
    uint16_t low = 0;
    uint16_t high = 1;
    std::vector<uint32_t> shape = {parameterNum,};
    std::string dtype = TypeNameGet<int16_t> ();

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

  /*Define reward function*/
  float IotEnv::GetReward()
  {
    float reward = cur_reward;
    cur_reward=0;
    return reward;
  }

  /*Define extra info. Optional*/
  std::string IotEnv::GetExtraInfo()
  {
    bool is_att = is_attack[m_agentId] || m_is_attack ? 1: 0;
    std::string info = "IsAttack " + std::to_string(is_att);
    info += " FP " + std::to_string(fp);
    info +=  " FN " + std::to_string(fn);
    info +=  " TP " + std::to_string(tp);
    info +=  " TN " + std::to_string(tn);
    //std::string info = "Active Agent " + std::to_string(m_agentId);
    return info;
  }

  /*Execute received actions*/
  bool IotEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
  {
    bool is_att = is_attack[m_agentId] || m_is_attack ? 1: 0;
    Ptr<OpenGymBoxContainer<int32_t> > box = DynamicCast<OpenGymBoxContainer<int32_t>>(action);
    int pred_clear_traffic = box->GetValue(0),
        pred_anomaly =  box->GetValue(1);

    fp = 0;
    fn = 0;
    tp = 0;
    tn = 0;

    if (is_att && pred_anomaly) {
        tn = 1;
        cur_reward = reward_tn;
    }
    else if (is_att && pred_clear_traffic) {
      cur_reward = reward_fp;
      fp = 1;
    }
    else if (pred_anomaly) {
        cur_reward = reward_fn;
        fn = 1;
    }
    else {
      cur_reward = reward_tp;
      tp = 1; 
    }

    // printf("Attack %d Reward: %f\n", is_attack[m_agentId], cur_reward);
    m_application->PacketFlowClear();
    return true;
  }
}