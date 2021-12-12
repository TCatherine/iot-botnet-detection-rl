


class Communicator:
    def __init__(self, num_agent):
        self.num_agent= num_agent
        self.agents_list = []
        self.dict = {}

    def subscribe(self, new_agent):
        self.agents_list.append(new_agent)

    def send(self, id, value):
        self.dict[id] = int(value)

    def generate_feature(self):
        feature = [0] * self.num_agent
        for key in self.dict.keys():
            feature[key] = int(self.dict[key])

        self.dict.clear()
        return feature