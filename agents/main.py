#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from agent import Agent
from communicator import Communicator
from env_stack import EnvStack
import json
from threading import Thread
import mlflow
import time

class Configuretion:
    def __init__(self, config_path):
        config_file = open(config_path, "r")
        json_string = json.load(config_file)["GeneralInfo"]
        self.num_agent = json_string["Number Agents"]
        self.num_env = json_string["Number Env"]
        self.port = json_string["First Port"]
        self.n_features = json_string["Number Features"]
        self.is_enable_communication = json_string["IsEnableCommunication"]
        self.is_feature = json_string["IsFeature"]
        path_list = json_string["WeightPath"]
        idx = ((2 if self.is_feature else 1) if self.is_enable_communication else 0)
        self.weight_path = path_list[idx]
        self.experiment_name = f'Agents'
        self.experiment_name += (f'-with-communicat{1 if self.is_feature else 0}' if self.is_enable_communication else '')


def add_metrics(string, fp, fn, tp, tn):
    for str in string:
        words = str.split(' ')
        fp += int(words[3])
        fn += int(words[5])
        tp += int(words[7])
        tn += int(words[9])
    return fp, fn, tp, tn

def main_loop(idx, agent, envs, steps=201):
    fp, fn, tp, tn = 0, 0, 0, 0
    total_reward = 0
    local_reward = 0
    # agent.policy.save_onnx(path = 'model.onnx')
    obs = envs.reset(idx)
    is_attack = [0] * agent.nenvs
    for itr in range(steps):
        action = agent.select_action(list(obs), is_attack)
        next_obs, reward, done, info = envs.step(idx, action)
        is_attack = [(int(i.split(' ')[1])) for i in info]
        total_reward += sum(reward)
        local_reward += sum(reward)

        loss = agent.train(action, reward, obs)

        fp, fn, tp, tn = add_metrics(info, fp, fn, tp, tn)
        if agent.is_enable_com:
            agent.share_knowledge()

        if itr and itr % 1 == 0:
            print('[Agent {}] Step {} Loss: {} Reward: {}({})' .format(idx, itr+1, loss, agent.reward, total_reward))
            mlflow.log_metric(f"loss {idx}", loss.tolist())
            mlflow.log_metric(f"reward {idx}", total_reward)
            if itr and itr % 20 == 0:
                agent.eps -= 0.05 if agent.eps > 0.05 else 0.05
                mlflow.log_metric(f"local reward {idx}", local_reward)
                mlflow.log_metric(f"false positive {idx}", fp)
                mlflow.log_metric(f"false negative {idx}", fn)
                mlflow.log_metric(f"true positive {idx}", tp)
                mlflow.log_metric(f"true negative {idx}", tn)
                agent.policy.save_model(agent.policy.path)
                fp, fn, tp, tn = 0, 0, 0, 0
                local_reward = 0

    envs.close(idx)

def init_mlflow(experiment_name):
    mlflow.set_tracking_uri('mlruns/')
    try:
        mlflow.create_experiment(experiment_name)
    except:
        print(f'Experiment {experiment_name} already exist')
    mlflow.set_experiment(experiment_name)

def single_thread_pipeline(idx, agent, envs):
    print('idx ', idx)
    envs.ini_simulation(idx)
    main_loop(idx, agent, envs)

if __name__ == "__main__":
    config_path = "../config.json"
    conf = Configuretion(config_path)
    linker = Communicator(conf.n_features)

    agent = Agent(conf.n_features, conf.num_env, conf.is_enable_communication,
                  conf.is_feature, Communicator(conf.n_features), conf.weight_path)
    envs = EnvStack(conf)
    time.sleep(15)

    init_mlflow(conf.experiment_name)
    with mlflow.start_run():
        list_threads = [Thread(target=single_thread_pipeline, args=(idx, agent, envs))
                        for idx in range(conf.num_agent)]
    for thread in list_threads:
        thread.start()

    [thread.join() for thread in list_threads]
