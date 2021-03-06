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

#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include "ns3/opengym-module.h"
#include "ns3/nstime.h"
#include "ns3/sequence-number.h"
#include "ns3/event-id.h"

#include "ns3/object.h"
#include "ns3/wifi-module.h"
#include "ns3/node-list.h"
#include "ns3/trace-helper.h"
#include "ns3/point-to-point-module.h"

#include "env.h"


#define MAX_STEP 30000

extern bool *is_attack;
extern int num_env;
namespace ns3 {

  struct features {
    uint8_t addrs[16];
    uint16_t port;
    uint16_t packet_number;
    uint16_t average_size_packet;
    uint8_t average_ttl;
    uint16_t s_time;
    uint16_t l_time;
    uint16_t duration;
    uint16_t average_interval;
    uint16_t max_deviation_interval;      
  };

  class IApplicationBase {
    public:
      virtual void PacketFlowConvert() = 0;
      virtual void PacketFlowClear() = 0;
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
    bool m_is_attack;
    Ipv4Address m_address;
    IApplicationBase* m_application;

  private:
    void ScheduleNextStateRead();
    float cur_reward;
    uint32_t m_agentId;
    uint32_t fp, fn, tp, tn;
    Time m_interval;
    
    std::string name_file = "dataset.csv";
    std::ofstream dataset_file;
  };

}

#endif // MY_GYM_ENTITY_H
