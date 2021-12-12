from policy import SimplePolicyArchitecture, CommunicationPolicyArchitecture
import torch
import numpy as np


class Agent:
    def __init__(self, id, port, number_features, is_enable_communication, is_feature, linker, weight_path):
        self.Q_value = None
        self.last_action = [0, 0]

        self.gamma = 0.02
        self.eps = 0.025

        self.id = id
        self.port = port
        self.is_enable_com = is_enable_communication
        self.is_feature = is_feature
        self.linker = (linker if is_enable_communication else None)

        # self.linker.subscribe(self)
        # self.share_knowledge()

        self.total_reward = 0
        self.reward = 0
        self.number_features = number_features + (1 if self.is_enable_com else 0)
        self.max_number_flows = 20

        if not self.is_enable_com:
            self.policy = SimplePolicyArchitecture(self.number_features, self.max_number_flows, weight_path)
        else:
            self.policy = CommunicationPolicyArchitecture(self.number_features, self.max_number_flows, weight_path, self.is_feature)
        self.learning_rate = 0.005
        self.loss = torch.nn.MSELoss()
        # self.loss = torch.nn.CrossEntropyLoss()
        # self.loss = nn.SmoothL1Loss()
        self.optimizer = torch.optim.Adam(self.policy.parameters(), lr=self.learning_rate)

        self.fp, self.fn, self.tp, self.tn = 0, 0, 0, 0

    def init_step(self, obs, reward):
        pass

    def epsilon_greedy(self, Q_value):
        if np.random.random() < self.eps:
            j = np.random.choice([0, 1])
        else:
            j = int(Q_value[0].argmax())
        self.last_action = [0, 0]
        self.last_action[j] = 1
        return self.last_action


    def select_action(self, obs):
        self.Q_value = self.predict_Q_value(obs)
        Q_value = self.Q_value[:2]

        last_action = self.epsilon_greedy(Q_value)
        return last_action

    def predict_Q_value(self, obs):
        if self.is_enable_com and len(obs):
            obs.extend([0]*(len(obs)//(self.number_features-1)))

        while len(obs)//self.number_features < self.max_number_flows:
            obs.extend([0]*self.number_features)

        obs = np.reshape(obs, (-1, self.number_features)).tolist()
        if self.is_enable_com:
            for o, f in zip(obs, self.linker.generate_feature()):
                o[len(o) - 1] = f

        obs = torch.tensor(obs, dtype=torch.float32).reshape(1, -1, self.number_features)
        obs = torch.transpose(obs, 1, 2)
        obs_np = obs.detach().numpy()
        result = self.policy.forward(obs)
        result_np = result.detach().numpy()

        return result

    def add_metrics(self, string):
        words = string.split(' ')
        self.fp += int(words[1])
        self.fn += int(words[3])
        self.tp += int(words[5])
        self.tn += int(words[7])

    def train(self, action, reward, next_obs):
        self.reward = reward
        self.total_reward += reward

        q_next = self.predict_Q_value(next_obs)
        q_next = reward + self.gamma * q_next.max()
        # self.Q_value.requires_grad = True
        target_Q = self.Q_value.detach().clone()
        target_Q[action.index(1)] = q_next
        # target_Q.no_grad()
        target_Q = target_Q.detach()
        # target_Q.requires_grad = False

        loss = self.loss(self.Q_value, target_Q)
        loss.backward()
        self.optimizer.step()
        return loss

    def share_knowledge(self):
        if not self.is_enable_com:
            return

        if self.is_feature:
            value = self.Q_value.detach().numpy()
            self.linker.send(self.id, value[2])
        else:
            self.linker.send(self.id, self.last_action.index(max(self.last_action)))