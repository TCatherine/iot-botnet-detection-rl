/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Technische Universität Berlin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Piotr Gawlowicz <gawlowicz@tkn.tu-berlin.de>
 */


#ifndef MY_GYM_ENTITY_H
#define MY_GYM_ENTITY_H

#include "ns3/opengym-module.h"
#include "ns3/nstime.h"
#include "ns3/sequence-number.h"
#include "ns3/ipv4-address.h"
#include "ns3/event-id.h"

#define MAX_STEP 10

extern int is_attack;

namespace ns3 {

struct features {
	uint32_t size;
	uint32_t uid;
	uint8_t *buffer;
  uint16_t sourcePort;        
  uint16_t destinationPort;   
  SequenceNumber32 sequenceNumber;  
  SequenceNumber32 ackNumber;       
  uint8_t length;             
  uint8_t flags;              
  uint16_t windowSize;        
  uint16_t urgentPointer;        
};

class IotEnv : public OpenGymEnv
{
public:
  IotEnv ();
  IotEnv (uint32_t agentId, Time stepTime);
  virtual ~IotEnv ();
  static TypeId GetTypeId (void);
  void DoDispose ();

  virtual Ptr<OpenGymSpace> GetActionSpace();
  Ptr<OpenGymSpace> GetObservationSpace();
  bool GetGameOver();
  Ptr<OpenGymDataContainer> GetObservation();
  float GetReward();
  std::string GetExtraInfo();
  bool ExecuteActions(Ptr<OpenGymDataContainer> action);

  std::vector<struct features> vector_features;
  EventId m_sendEvent;
  uint32_t m_port;
  Ipv4Address m_address;
private:
  void ScheduleNextStateRead();
  uint16_t Epsilon_Greedy(std::vector<int> action);
  float cur_reward;
  float eps = 0.05;
  uint32_t m_agentId;
  uint32_t fp, fn, tp, tn;
  Time m_interval;
};
}


#endif // MY_GYM_ENTITY_H
