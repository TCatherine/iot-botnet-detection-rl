import torch.nn as nn
import torch
from threading import Lock
import os
import torch.onnx

data_lock = Lock()


class RecurrentPolicyArchitecture(nn.Module):
    def __init__(self, num_feature, weight_path=None):
        super().__init__()

        self.num_feature = num_feature

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.path = weight_path

        # self.input_bn = nn.BatchNorm1d(num_features=self.num_feature)
        layers_num = 1
        hidden_size = 16
        self.lstm = nn.LSTM(
            input_size=5,
            hidden_size=hidden_size,
            num_layers=layers_num,
            batch_first=True
        )

        self.state_value = nn.Sequential(
            nn.Linear(layers_num * hidden_size, 32),
            nn.LeakyReLU(),
            nn.Linear(32, 1),
        )
        self.action_value = nn.Sequential(
            nn.Linear(layers_num * hidden_size, 32),
            nn.LeakyReLU(),
            nn.Linear(32, 2),
        )

        self.to(self.device)

    def forward(self, obs, com_data=None):
        batch_size = obs.shape[0]
        # out = self.input_bn(out)
        # rnn_out: Batch * Length * Hidden_size
        # h_n: num_layers * batch * Hidden_size
        rnn_out, (h_n, c_n) = self.lstm(obs)
        # h = rnn_out.mean(axis=1)
        h = h_n.transpose(0, 1)
        h = h.reshape(batch_size, -1)
        # $A$
        action_value = self.action_value(h)
        # $V$
        state_value = self.state_value(h)

        # $A(s, a) - \frac{1}{|\mathcal{A}|} \sum_{a' \in \mathcal{A}} A(s, a')$
        action_score_centered = action_value - action_value.mean(dim=-1, keepdim=True)
        # $Q(s, a) =V(s) + \Big(A(s, a) - \frac{1}{|\mathcal{A}|} \sum_{a' \in \mathcal{A}} A(s, a')\Big)$
        q = state_value + action_score_centered

        return q

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
        # model.eval()


class Conv1DPolicyArchitecture(nn.Module):
    def __init__(self, num_feature, weight_path=None):
        super().__init__()

        self.num_feature = num_feature

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.path = weight_path

        self.input_bn = nn.BatchNorm1d(num_features=self.num_feature)
        self.conv1 = nn.Sequential(
            nn.Conv1d(in_channels=num_feature,
                      out_channels=8,
                      kernel_size=3,
                      padding=1),
            nn.LeakyReLU()
        )

        self.conv2 = nn.Sequential(
            nn.Conv1d(in_channels=8,
                      out_channels=16,
                      kernel_size=3,
                      padding=1),
            nn.LeakyReLU()
        )

        self.state_value = nn.Sequential(
            nn.Linear(16, 32),
            nn.LeakyReLU(),
            nn.Linear(32, 1),
        )
        self.action_value = nn.Sequential(
            nn.Linear(16, 32),
            nn.LeakyReLU(),
            nn.Linear(32, 2),
        )

        self.to(self.device)

    def forward(self, obs, com_data=None):
        out = torch.tensor(obs, dtype=torch.float32, device=self.device)

        out = self.input_bn(out)
        out = self.conv1(out)
        out = self.conv2(out)

        h = torch.max(out, axis=2).values

        # $A$
        action_value = self.action_value(h)
        # $V$
        state_value = self.state_value(h)

        # $A(s, a) - \frac{1}{|\mathcal{A}|} \sum_{a' \in \mathcal{A}} A(s, a')$
        action_score_centered = action_value - action_value.mean(dim=-1, keepdim=True)
        # $Q(s, a) =V(s) + \Big(A(s, a) - \frac{1}{|\mathcal{A}|} \sum_{a' \in \mathcal{A}} A(s, a')\Big)$
        q = state_value + action_score_centered

        return q

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
        # model.eval()


class CommunicationPolicyArchitecture(nn.Module):
    def __init__(self, num_feature, max_flow, weight_path, is_feature):
        super().__init__()

        self.num_feature = num_feature
        self.max_flow = max_flow

        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        self.is_feature = is_feature

        self.path = weight_path
        self.con1 = nn.Conv1d(in_channels=num_feature, out_channels=32, kernel_size=2, stride=2)
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
        # model.eval()
