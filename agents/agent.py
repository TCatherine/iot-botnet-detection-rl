from policy import RecurrentPolicyArchitecture, Conv1DPolicyArchitecture
import torch
import numpy as np
from random import sample
import pandas as pd


class ExperienceReplay:
    def __init__(self):
        self.m = []

    def add(self, s, a, r, sn):
        self.m.append((s, a, r, sn))

        if len(self.m) > 1000:
            self.m.pop(0)

    def sample(self, batch=10):
        batch = sample(self.m, batch)
        s, a, r, sn = zip(*batch)
        return s, a, r, sn

    def __len__(self):
        return len(self.m)

    def save(self, path='experience.pickle'):
        d = pd.DataFrame(self.m)
        d.to_pickle(path)

    def load(self, path='experience.pickle'):
        d = pd.read_pickle(path)
        self.m = d.to_records(index=False).tolist()


class Agent:
    def __init__(self,
                 number_features,
                 num_envs,
                 is_feature,
                 linker,
                 weight_path):
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.nenvs = num_envs
        self.Q_value = None
        self.last_action = [[0, 0] for _ in range(self.nenvs)]

        self.gamma = 0.9
        self.eps = 0
        self.eps_delta = 0.01
        self.learning_rate = 0.03
        self.action_space = 2

        self.is_feature = is_feature
        self.linker = linker

        self.total_reward = 0
        self.reward = 0
        self.number_features = number_features

        self.policy_net = Conv1DPolicyArchitecture(self.number_features)
        self.target_net = Conv1DPolicyArchitecture(self.number_features)
        self.update_target_weights()

        self.loss = torch.nn.SmoothL1Loss()
        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=self.learning_rate)

        self.fp, self.fn, self.tp, self.tn = 0, 0, 0, 0
        self.em = ExperienceReplay()

    def init_step(self, obs, reward):
        pass

    def epsilon_greedy(self, Q_pred):
        last_action = torch.zeros(self.nenvs, self.action_space, dtype=torch.int32, device=self.device)
        for i, Q_value in enumerate(Q_pred):
            if np.random.random() < self.eps:
                j = np.random.choice([0, 1])
                print('.', end='')
            else:
                j = int(Q_value.argmax())
            last_action[i][j] = 1

        return last_action

    def select_action(self, obs):
        with torch.no_grad():
            self.policy_net.eval()
            obs = self.prepare_state(obs)
            Q_value = self.policy_net(obs)
            is_attack_pred = bool(Q_value[0, 0] > Q_value[0, 1])
            self.policy_net.train()

        last_action = self.epsilon_greedy(Q_value)
        return last_action.detach().cpu().numpy()

    def train(self) -> torch.Tensor:
        s, a, r, sn = self.em.sample(batch=100)
        s = self.prepare_state(s)
        sn = self.prepare_state(sn)

        a = torch.tensor(a, device=self.device)[:, 0, :]
        r = torch.tensor(r, device=self.device)[:, 0]

        self.optimizer.zero_grad()
        q_next = self.target_net(sn).detach()
        q_next_max = q_next.max(axis=1).values
        q_next_max += r

        q_pred = self.policy_net(s)
        # a = torch.stack([q_pred, r], axis=1)
        q_pred_to_optimize = torch.gather(q_pred, 1, a.max(axis=1).indices.reshape(-1, 1))[:, 0]

        loss = self.loss(q_pred_to_optimize, q_next_max)
        loss.backward()
        self.optimizer.step()
        with torch.no_grad():
            new_q = self.policy_net(s)
            delta = (torch.stack([q_next_max,
                                  q_pred_to_optimize,
                                  (new_q * a).sum(axis=1)]).T).detach().cpu().numpy()
        q_pred_np = q_pred.detach().cpu().numpy()
        new_q = new_q.detach().cpu().numpy()
        q_next_np = q_next.detach().cpu().numpy()
        a_np = a.detach().cpu().numpy()
        r_np = r.reshape(-1, 1).detach().cpu().numpy()
        d = [q_pred_np, new_q, q_next_np, r_np, a_np]
        return loss

    def prepare_state(self, s):
        if len(s) == 1 and len(s[0]) == 0:
            s = [[0] * self.number_features * 2]
        if len(s) == 1 and len(s[0]) == 5:
            s = [s[0] + [0] * self.number_features]
        st = [torch.tensor(si).reshape(1, -1, self.number_features)[0] for si in s]
        st = torch.nn.utils.rnn.pad_sequence(st, batch_first=True)
        st = torch.tensor(st, dtype=torch.float32, device=self.device)

        # bs, le, _ = st.shape
        # mu = torch.tensor([39.0697, 354.2316, 7695.0913, 1052.2882, 2002.2435], device='cuda:0')
        # mu.repeat((bs, le, 1))
        # std = torch.tensor([112.0217, 433.0179, 10802.2002, 1937.2877, 4100.1074], device='cuda:0')
        # std = std.repeat((bs, le, 1))
        # st = (st - mu) / std
        return st.transpose(2, 1)  # uncomment if conv1d

    def update_target_weights(self):
        self.target_net.load_state_dict(self.policy_net.state_dict())
