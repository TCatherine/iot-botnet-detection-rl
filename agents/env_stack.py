import numpy as np
import multiprocessing as mp
from ns3gym import ns3env
import pty
from subprocess import Popen
import threading
import os
import os
import sys
import select
import termios
import tty

def worker(main_e, work_e, env, idx):
    main_e.close()
    while True:
        cmd, data = work_e.recv()
        if cmd == 'ini_simulation':
            env.init_simulation(idx)
        elif cmd == 'reset':
            env.reset(idx)
        elif cmd == 'close':
            env.close(idx)
        elif cmd == 'step':
            res = env.step(idx, data)
            d = [] if res[0] is None else [*res[0]]
            work_e.send(d)
            work_e.send(res[1])
            work_e.send(res[2])
            work_e.send(res[3])

class AgentEnvWrapper:
    def __init__(self, port, id):
        self.port = port
        self.id = id
        self.ns3 = None

    def init_simulation(self):
        self.ns3 = ns3env.Ns3Env(port=self.port, startSim=False)

    def reset(self):
        obs = self.ns3.reset()
        obs = (0 if obs is None else obs)
        return obs

    def step(self, data):
        res = self.ns3.step(data)
        return res

    def close(self):
        return self.ns3.close()


class EnvWrapper:
    def __init__(self, port, nagents):
        self.port = port
        self.nagents = nagents

        os.chdir('./../ns-3.29/')
        print(os.getcwd())
        cmd = f'./waf --run "my_env {self.port}"'
        th = threading.Thread(target=os.system, args=(cmd,))
        th.start()

        self.agent_env = [AgentEnvWrapper(self.port + i, i+1) for i in range(self.nagents)]

    def init_simulation(self, idx):
        self.agent_env[idx].init_simulation()

    def reset(self, idx):
        return self.agent_env[idx].reset()

    def step(self, idx, data):
        return self.agent_env[idx].step(data)

    def close(self, idx):
        return self.agent_env[idx].close()


class EnvStack:
    def __init__(self, conf):
        envs = [EnvWrapper(conf.port+conf.num_agent*i, conf.num_agent) for i in range(conf.num_env)]

        self.nenv = conf.num_env
        self.main_end = []
        self.work_end = []

        for _ in range(conf.num_agent):
            temp_main = []
            temp_work = []
            for _ in range(conf.num_env):
                p, c = mp.Pipe()
                temp_main.append(p)
                temp_work.append(c)
            self.main_end.append(temp_main)
            self.work_end.append(temp_work)

        self.ps = []
        idx = 0
        for temp_main, temp_work, i in zip(self.main_end, self.work_end, range(conf.num_agent)):
            pa = []
            for m, w, env in zip(temp_main, temp_work, envs):
                p = mp.Process(target=worker, args=(m, w, env, i))
                idx+=1
                pa.append(p)
            self.ps.append(pa)

        for pa in self.ps:
            for p in pa:
                p.start()

    def reset(self, idx):
        for m in self.main_end[idx]:
            m.send(('reset', None))
        # obs = [m.recv() for m in self.main_end]
        # obs = [[] if ob is None else ob for ob in obs]
        return [[] for _ in range(self.nenv)]

    # def plan(self):
    #     for m in self.main_end:
    #         m.send(('plan', None))
    #     results = [m.recv() for m in self.main_end]

    def step(self, idx, actions):
        obs, reward, is_right, metrics = [], [], [], []
        for m, a in zip(self.main_end[idx], actions):
            m.send(('step', a))

        for m in self.main_end[idx]:
            obs.append(m.recv())
            reward.append(m.recv())
            is_right.append(m.recv())
            metrics.append(m.recv())

        return obs, reward, is_right, metrics

    def close(self, idx):
        for m in self.main_end[idx]:
            m.send(('close', None))
        for proc in self.ps[idx]:
            proc.join()


    def ini_simulation(self, idx):
        for m in self.main_end[idx]:
            m.send(('ini_simulation', None))
