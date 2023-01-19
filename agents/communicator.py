#!/usr/bin/env python
import numpy as np


class Communicator:
    def __init__(self, num_agent):
        self.num_agent = num_agent
        self.agents_list = []
        self.dict = {}

    def send(self, id, value):
        self.dict[id] = value

    def generate_feature(self):
        feature = np.zeros((self.num_agent, 2))
        for key in self.dict.keys():
            feature[key] = self.dict[key]

        self.dict.clear()
        return feature

    def empty_state(self):
        pass
