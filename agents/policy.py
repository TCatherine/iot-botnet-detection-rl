import torch.nn as nn
import torch
from threading import Lock
import os
import torch.onnx

data_lock = Lock()

class SimplePolicyArchitecture(nn.Module):
    def __init__(self, num_feature, max_flow, weight_path):
        super().__init__()

        self.num_feature = num_feature
        self.max_flow = max_flow

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.path = weight_path


        self.con1 = nn.Conv1d(in_channels=num_feature,  out_channels=32, kernel_size=2, stride=2)
        self.fun1 = nn.LeakyReLU()

        self.pool1 = nn.MaxPool1d(kernel_size=4, stride=4)
        # self.con2 = nn.Conv1d(in_channels=32, out_channels=64, kernel_size=2, stride=2)
        # self.fun2 = nn.LeakyReLU()


        self.fully_connected = nn.Sequential(nn.Linear(32, 64), nn.LeakyReLU())
        self.drop1 = nn.Dropout()
        self.fully_connected_2 = nn.Sequential(nn.Linear(64, 32), nn.LeakyReLU())
        self.drop2 = nn.Dropout()
        self.last_layer = nn.Linear(32, 2)
        self.fun2 = nn.LeakyReLU()
        self.load_model(self.path)

    def forward(self, objects, com_data = None):
        object = torch.tensor(objects, dtype=torch.float32, device=self.device)
        res1 = self.con1(object)
        res2 = self.fun1(res1)
        res3 = self.pool1(res2)
        # res4 = self.con2(res3)
        # res5 = self.fun2(res4)
        res6 = torch.mean(res3, 2)

        full_conncection_1 = self.fully_connected(res6)
        drop1 = self.drop1(full_conncection_1)
        full_conncection_2 = self.fully_connected_2(drop1)
        drop2 = self.drop1(full_conncection_2)
        last_layer = self.last_layer(drop2)
        pos_values = self.fun2(last_layer)

        result = pos_values.reshape(-1, 2)
        return result

    def save_model(self, path):
        with data_lock:
            torch.save(self.state_dict(), path)

    def save_onnx(self, path):
        x = torch.randn(1, self.num_feature, self.max_flow, requires_grad=True)
        torch_out = self(x)

        # Export the model
        torch.onnx.export(self,  # model being run
                          x,  # model input (or a tuple for multiple inputs)
                          path,  # where to save the model (can be a file or file-like object)
                          export_params=True,  # store the trained parameter weights inside the model file
                          opset_version=10,  # the ONNX version to export the model to
                          do_constant_folding=True,  # whether to execute constant folding for optimization
                          input_names=['input'],  # the model's input names
                          output_names=['output'],  # the model's output names
                          dynamic_axes={'input': {0: 'batch_size'},  # variable length axes
                                        'output': {0: 'batch_size'}})

    def load_model(self, path):
        with data_lock:
            if os.path.isfile(path):
                self.load_state_dict(torch.load(path))
        #model.eval()

class CommunicationPolicyArchitecture(nn.Module):
    def __init__(self, num_feature, max_flow, weight_path, is_feature):
        super().__init__()

        self.num_feature = num_feature
        self.max_flow = max_flow

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.is_feature = is_feature

        self.path = weight_path
        self.con1 = nn.Conv1d(in_channels=num_feature,  out_channels=32, kernel_size=2, stride=2)
        self.fun1 = nn.LeakyReLU()
        self.drop1 = nn.Dropout()
        self.pool1 = nn.MaxPool1d(kernel_size=4, stride=4)
        # self.con2 = nn.Conv1d(in_channels=32, out_channels=64, kernel_size=2, stride=2)
        self.fun2 = nn.LeakyReLU()


        self.fully_connected = nn.Sequential(nn.Linear(32, 64), nn.LeakyReLU())
        self.drop2 = nn.Dropout()
        self.last_layer = (nn.Linear(64, 3) if self.is_feature else nn.Linear(64, 2))
        self.fun2 = nn.LeakyReLU()

        self.load_model(self.path)

    def forward(self, objects):
        object = torch.tensor(objects, dtype=torch.float32, device=self.device)
        res1 = self.con1(object)
        res2 = self.fun1(res1)
        res3 = self.pool1(res2)
        # res4 = self.con2(res3)
        # res5 = self.fun2(res4)
        res6 = torch.mean(res3, 2)

        full_conncection = self.fully_connected(res6)
        drop = self.drop2(full_conncection)
        last_layer = self.last_layer(drop)
        pos_values = self.fun2(last_layer)

        result = pos_values.reshape(-1)
        return result

    def save_model(self, path):
        with data_lock:
            torch.save(self.state_dict(), path)

    def save_onnx(self, path):
        x = torch.randn(1, self.num_feature, self.max_flow, requires_grad=True)
        torch_out = self(x)

        # Export the model
        torch.onnx.export(self,  # model being run
                          x,  # model input (or a tuple for multiple inputs)
                          path,  # where to save the model (can be a file or file-like object)
                          export_params=True,  # store the trained parameter weights inside the model file
                          opset_version=10,  # the ONNX version to export the model to
                          do_constant_folding=True,  # whether to execute constant folding for optimization
                          input_names=['input'],  # the model's input names
                          output_names=['output'],  # the model's output names
                          dynamic_axes={'input': {0: 'batch_size'},  # variable length axes
                                        'output': {0: 'batch_size'}})

    def load_model(self, path):
        with data_lock:
            if os.path.isfile(path):
                self.load_state_dict(torch.load(path))
        #model.eval()