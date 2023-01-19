import numpy as np
import torch

from communicator import Communicator
from policy import Conv1DPolicy
from experience_replay import ExperienceReplay


class Agent:
    def __init__(self,
                 idx: int,
                 num_features: int,
                 num_envs: int,
                 em: ExperienceReplay
                 ):
        self.em = em

        self.idx = idx
        self.num_features = num_features
        self.num_envs = num_envs

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.Q_value = None
        self.last_action = [[0, 0] for _ in range(self.num_envs)]

        self.gamma = 0.9
        self.eps = 0.05
        self.eps_delta = 0.01
        self.learning_rate = 0.03
        self.action_space = 2

        self.total_reward = 0
        self.reward = 0

        self.fp, self.fn, self.tp, self.tn = 0, 0, 0, 0
        self.loss = torch.nn.SmoothL1Loss()

        self.policy_net = Conv1DPolicy(inp_feats_num=self.num_features, out_feats_num=2)
        self.target_net = Conv1DPolicy(inp_feats_num=self.num_features, out_feats_num=2)
        self.update_target_weights()

        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=self.learning_rate)

    def update_target_weights(self):
        self.target_net.load_state_dict(self.policy_net.state_dict())

    def epsilon_greedy(self, Q_pred):
        last_action = torch.zeros(self.num_envs, self.action_space, dtype=torch.int32, device=self.device)
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
            self.policy_net.train()

        last_action = self.epsilon_greedy(Q_value)

        q_values_np = Q_value.cpu().numpy()
        return last_action.detach().cpu().numpy(), q_values_np

    def flatten_states(self, s):
        s_concat = []
        for si in s:
            for sj in si:
                s_concat.append(sj)
        return s_concat

    def train(self):
        s, a, r, sn = self.em.sample(batch=50)
        s = self.flatten_states(s)
        sn = self.flatten_states(sn)

        s = self.prepare_state(s)
        sn = self.prepare_state(sn)

        a = torch.tensor(a, device=self.device)[:, 0, :]
        r = torch.tensor(r, device=self.device)[:, 0]

        self.optimizer.zero_grad()
        q_next = self.target_net(sn).detach()
        q_next_max = q_next.max(axis=1).values
        q_next_max += r

        q_pred = self.policy_net(s)
        q_pred_to_optimize = torch.gather(q_pred, 1, a.max(axis=1).indices.reshape(-1, 1))[:, 0]

        loss = self.loss(q_pred_to_optimize, q_next_max)
        loss.backward()
        self.optimizer.step()

        return loss

    def prepare_state(self, s):
        st2 = [torch.tensor(si) for si in s]
        st2 = torch.nn.utils.rnn.pad_sequence(st2, batch_first=True)
        st2 = torch.tensor(st2, dtype=torch.float32, device=self.device)
        a = st2[0].cpu().numpy()
        return st2.transpose(2, 1)  # uncomment if conv1d

    def fix_non_valid_state(self, s):
        # we need sequence minimum length=2 because of convolution has window=3
        if len(s) == 1 and len(s[0]) == 0:
            s = [[0] * self.num_features * 2]
        if len(s) == 1 and len(s[0]) == 5:
            s = [s[0] + [0] * self.num_features]

        st = [np.array(si).reshape((-1, self.num_features)) for si in s]
        return st

    def add_communication_to_state(self, obs):
        return obs


class AgentSync(Agent):
    def __init__(self,
                 idx: int,
                 num_features: int,
                 num_envs: int,
                 is_feature: bool,
                 comm: Communicator,
                 em: ExperienceReplay
                 ):
        super().__init__(idx, num_features, num_envs, em)

        self.is_feature = is_feature
        self.comm = comm

        out_feats_num = 3 if self.is_feature else 2

        self.policy_net = Conv1DPolicy(inp_feats_num=self.num_features + 2, out_feats_num=out_feats_num)
        self.target_net = Conv1DPolicy(inp_feats_num=self.num_features + 2, out_feats_num=out_feats_num)
        self.update_target_weights()

        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=self.learning_rate)

    def select_action(self, obs):
        with torch.no_grad():
            self.policy_net.eval()
            obs = self.prepare_state(obs)
            Q_value = self.policy_net(obs)
            self.policy_net.train()

        last_action = self.epsilon_greedy(Q_value)

        q_values_np = Q_value.cpu().numpy()
        self.comm.send(self.idx, q_values_np)
        return last_action.detach().cpu().numpy(), q_values_np

    def add_communication_to_state(self, obs):
        # get comm feature
        comm_state = self.comm.generate_feature()
        comm_state = np.array(comm_state, dtype=np.float32)
        comm_state = comm_state.mean(axis=0)

        st2 = []
        for si in obs:
            packets_num = len(si)
            comm_state_rep = np.tile(comm_state, [packets_num, 1])
            st2.append(np.concatenate([si, comm_state_rep], axis=1))

        return st2
