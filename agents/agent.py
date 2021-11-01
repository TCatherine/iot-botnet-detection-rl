from policy import PolicyArchitecture
from torch import nn
import torch


class Agent:
    def __init__(self, id, port, ac_space, number_features):
        self.Q_value = 0
        self.alpha = 0.1
        self.gamma = 0.01
        self.id = id
        self.port = port
        self.space = ac_space
        self.last_action = 0
        self.total_reward = 0
        self.reward = 0
        self.number_features = number_features
       # self.observation = 0

        self.policy = PolicyArchitecture(number_features)
        self.learning_rate = 0.01
        self.loss = nn.SmoothL1Loss()
        self.optimizer = torch.optim.Adam(self.policy.parameters(), lr=self.learning_rate)

        self.fp, self.fn, self.tp, self.tn = 0, 0, 0, 0

    def init_step(self, obs, reward):
        pass

    def action_space(self, obs):
        if obs is None or len(obs) is 0:
            self.last_action = [50, 0]
            return self.last_action
        obs = torch.tensor(obs, dtype=torch.float32).reshape(-1, self.number_features)
        result = self.policy.forward(obs)
        result = result.tolist()
        action = [int(e) for e in result]
        #for elem in result:
         #   action_elem = [ int(e) for e in elem]
           # action.append(action_elem)
        self.last_action = action
        return action

    def add_metrics(self, string):
        words = string.split(' ')
        self.fp += int(words[1])
        self.fn += int(words[3])
        self.tp += int(words[5])
        self.tn += int(words[7])

        pass

    def step(self, obs, reward):
        self.last_action = self.action_space(obs)
        target_Q = torch.Tensor([reward + self.gamma * max(self.last_action)])
        self.Q_value += self.alpha*(target_Q - self.Q_value)
        self.reward = reward
        self.total_reward += reward
        self.observation = obs
        return target_Q

    def train(self, obs, reward):
        target_Q = self.step(obs, reward)
        loss = self.loss(self.Q_value, target_Q)
        loss.requires_grad = True
        loss.backward()
        self.optimizer.step()
        return loss
