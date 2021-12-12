#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ns3gym import ns3env
from agent import Agent
from communicator import Communicator
import json
from threading import Thread
import mlflow

class Configuretion:
    def __init__(self, config_path):
        config_file = open(config_path, "r")
        json_string = json.load(config_file)["GeneralInfo"]
        self.num_agent = json_string["Number Agents"]
        self.port = json_string["First Port"]
        self.n_features = json_string["Number Features"]
        self.is_enable_communication = json_string["IsEnableCommunication"]
        self.is_feature = json_string["IsFeature"]
        path_list = json_string["WeightPath"]
        idx = ((2 if self.is_feature else 1) if self.is_enable_communication else 0)
        self.weight_path = path_list[idx]
        self.experiment_name = f'Agents'
        self.experiment_name += (f'-with-communicat{1 if self.is_feature else 0}' if self.is_enable_communication else '')


def main_loop(env, agent, steps=101):
    agent.policy.save_onnx(path = 'model.onnx')
    obs = env.reset()
    for itr in range(steps):
        action = agent.select_action(list(obs))
        next_obs, reward, done, info = env.step(action)
        obs = list(next_obs)
        loss = agent.train(action, reward, list(next_obs))

        agent.add_metrics(info)
        if agent.is_enable_com:
            agent.share_knowledge()

        if itr and itr % 1 == 0:
            print('[Agent {}] Step {} Loss: {} Reward: {}({})' .format(agent.id, itr+1, loss, agent.reward, agent.total_reward))
            mlflow.log_metric(f"loss {agent.id}", loss.tolist())
            mlflow.log_metric(f"reward {agent.id}", agent.total_reward)
            if itr and itr % 10 == 0:
                mlflow.log_metric(f"false positive {agent.id}", agent.fp)
                mlflow.log_metric(f"false negative {agent.id}", agent.fn)
                mlflow.log_metric(f"true positive {agent.id}", agent.tp)
                mlflow.log_metric(f"true negative {agent.id}", agent.tn)
                agent.policy.save_model(agent.policy.path)
                agent.fp, agent.fn, agent.tp, agent.tn = 0, 0, 0, 0

    env.close()

def init_mlflow(experiment_name):
    mlflow.set_tracking_uri('mlruns/')
    try:
        mlflow.create_experiment(experiment_name)
    except:
        print(f'Experiment {experiment_name} already exist')
    mlflow.set_experiment(experiment_name)


def generate_envs(idx, linker, conf):
    cur_port = conf.port + idx
    env = ns3env.Ns3Env(port=cur_port, startSim=False)
    agent = Agent(idx, conf.port, conf.n_features, conf.is_enable_communication,
                  conf.is_feature, linker, conf.weight_path)
    print(f'Agent {idx} is activate')
    return env, agent

def single_thread_pipeline(num_agent, linker, conf):
        env, agent = generate_envs(num_agent, linker, conf)
        main_loop(env, agent)

if __name__ == "__main__":
    config_path = "config.json"
    conf = Configuretion(config_path)

    init_mlflow(conf.experiment_name)
    with mlflow.start_run():
        linker = Communicator(conf.n_features)
        list_threads = [Thread(target=single_thread_pipeline, args=(idx, linker, conf))
                        for idx in range(conf.num_agent)]

    [thread.start() for thread in list_threads]
    [thread.join() for thread in list_threads]
