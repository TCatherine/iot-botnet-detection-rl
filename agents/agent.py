from policy import SimplePolicyArchitecture, CommunicationPolicyArchitecture
import torch
import numpy as np


class Agent:
    def __init__(self, number_features, num_envs, is_enable_communication, is_feature, linker, weight_path):
        self.nenvs = num_envs
        self.Q_value = None
        self.last_action = [[0, 0] for _ in range(self.nenvs)]

        self.gamma = 0.0003
        self.eps = 0.4
        self.learning_rate = 0.001

        self.is_enable_com = is_enable_communication
        self.is_feature = is_feature
        self.linker = (linker if is_enable_communication else None)

        self.total_reward = 0
        self.reward = 0
        self.number_features = number_features + (1 if self.is_enable_com else 0)
        self.max_number_flows = 500

        if not self.is_enable_com:
            self.policy = SimplePolicyArchitecture(self.number_features, self.max_number_flows, weight_path)
        else:
            self.policy = CommunicationPolicyArchitecture(self.number_features, self.max_number_flows,
                                                          weight_path, self.is_feature)

        # self.loss = torch.nn.MSELoss()
        self.loss = torch.nn.CrossEntropyLoss(reduction='mean')
        # self.loss = nn.SmoothL1Loss()
        self.optimizer = torch.optim.Adam(self.policy.parameters(), lr=self.learning_rate)

        self.fp, self.fn, self.tp, self.tn = 0, 0, 0, 0

    def init_step(self, obs, reward):
        pass

    def epsilon_greedy(self, Q_values, is_attack):
        for i, Q_value in enumerate(Q_values):
            if np.random.random() < self.eps:
                # j = np.random.choice([0, 1])
                j = is_attack[i]
            else:
                j = int(Q_value.argmax())
                # print(f'[Agent {self.id}] Choosen action {j} ({Q_value.detach().numpy()})')
            self.last_action[i] = [0, 0]
            self.last_action[i][j] = 1

        return self.last_action


    def select_action(self, obs, is_attack):
        self.Q_value = self.predict_Q_value(obs)
        # Q_value = self.Q_value[:2]

        last_action = self.epsilon_greedy(self.Q_value, is_attack)
        return last_action

    def predict_Q_value(self, obs):
        for i in range(len(obs)):
            if self.is_enable_com and len(obs[i]) and len(obs[i]) % (self.number_features-1)==0:
                obs[i].extend([0]*(len(obs[i])//(self.number_features-1)))
            # elif len(obs[i]) and len(obs[i]) % self.number_features==0:
            #     obs[i].extend([0]*(len(obs[i])//self.number_features))

            k = self.max_number_flows - len(obs[i]) // self.number_features
            obs[i].extend([0] * self.number_features * k)
        obs = np.reshape(obs, (-1, self.max_number_flows, self.number_features)).tolist()
        if self.is_enable_com:
            for o, f in zip(obs, self.linker.generate_feature()):
                o[len(o) - 1] = f

        obs = torch.tensor(obs, dtype=torch.float32).reshape(-1, self.max_number_flows, self.number_features)
        obs = torch.transpose(obs, 1, 2)
        result = self.policy.forward(obs)

        return result


    def train(self, action, reward, next_obs):
        self.reward = reward
        self.total_reward += sum(reward)

        q_next = self.predict_Q_value(next_obs)
        q_next_max = [r + q.max()*self.gamma for q, r in zip(q_next, reward)]
        target_Q = self.Q_value.detach().clone()
        for i in range(len(action)):
            target_Q[i][action[i].index(1)]=q_next_max[i]
        target_Q = target_Q.detach()

        loss = self.loss(self.Q_value, torch.max(target_Q, 1)[1])
        loss.backward()
        self.optimizer.step()
        return loss


    def share_knowledge(self):
        if not self.is_enable_com:
            return

        # if self.is_feature:
        #     value = self.Q_value.detach().numpy()
        #     self.linker.send(self.id, value[2])
        # else:
        #     self.linker.send(self.id, self.last_action.index(max(self.last_action)))