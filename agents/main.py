#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ns3gym import ns3env
from agent import Agent
import json
from threading import Thread
from result import *

def init_space(env, agent):
    obs = env.reset()
    action = agent.action_space(obs)
    agent.Q_value = agent.learning_rate * max(action)
    #obs, reward, done, info = env.step(agent.last_action)
    #loss = agent.train(obs, reward)
    #print('Step {} Loss: {} Reward: {}({})'.format(0, loss, agent.reward, agent.total_reward))

def train(env, agent, save_data, iterations=300000):
    for itr in range(iterations):
        obs, reward, done, info = env.step(agent.last_action)
        agent.add_metrics(info)
        loss = agent.train(obs, reward)
        loss.requires_grad = False

        if itr and itr % 100 == 0:
            print('[Agent {}] Step {} Loss: {} Reward: {}({})' .format(agent.id, itr+1, loss, agent.reward, agent.total_reward))
            agent.policy.save_model()
            save_data.save_loss(agent.id, loss)
            save_data.save_reward(agent.id, agent.total_reward)
            save_data.save_metrics(agent.id, agent.fp, agent.fn, agent.tp, agent.tn)
            agent.total_reward = 0
            agent.fp, agent.fn, agent.tp, agent.tn = 0, 0, 0, 0

    env.close()

def generate_envs(idx, port, n_features):
    cur_port = port + idx
    env = ns3env.Ns3Env(port=cur_port, startSim=False)
    ac_space = env.action_space
    agent = Agent(idx, port, ac_space, n_features)
    init_space(env, agent)
    return env, agent

def single_thread_pipeline(num_agent, port, n_features, save_data):
    env, agent = generate_envs(num_agent, port, n_features)
    train(env, agent, save_data)

if __name__ == "__main__":
    config_file = open("config.json", "r")
    json_string = json.load(config_file)["GeneralInfo"]
    num_agent = json_string["Number Agents"]
    port = json_string["First Port"]
    n_features = json_string["Number Features"]
    list_threads = []
    save_data = SaveResult(num_agent)
    for idx in range(num_agent):
        thread = Thread(target=single_thread_pipeline, args=(idx, port, n_features, save_data))
        list_threads.append(thread)
    [thread.start() for thread in list_threads]
    [thread.join() for thread in list_threads]
