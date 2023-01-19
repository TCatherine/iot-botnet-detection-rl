from random import sample

import pandas as pd


class ExperienceReplay:
    def __init__(self, save_path):
        self.m = []
        self.base_path = '/home/alexey/Workspace/iot-botnet-detection-rl/ns-3.29/'
        self.save_path = self.base_path + save_path
        self.load()

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

    def save(self):
        d = pd.DataFrame(self.m)
        d.to_pickle(self.save_path)

    def load(self):
        d = pd.read_pickle(self.save_path)
        self.m = d.to_records(index=False).tolist()
        self.m = self.m[200:]
