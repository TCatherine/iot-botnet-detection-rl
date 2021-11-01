import torch.nn as nn
import torch
import math
from threading import Lock
import os

data_lock = Lock()

class MultiHeadAttention(nn.Module):
    def __init__(self, heads, d_model, dropout=0.1):
        super().__init__()
        # probably it needs to be fixed
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        # feature number of each entity
        self.d_boxes = d_model

        self.d_model = d_model
        self.d_k = d_model // heads
        self.h = heads

        #for self-attention
        self.mask = self.masked()

        self.visibility = None

        self.q_linear = nn.Linear(self.d_boxes, d_model)
        self.v_linear = nn.Linear(self.d_boxes, d_model)
        self.k_linear = nn.Linear(self.d_boxes, d_model)

        self.dense = {'key': self.k_linear,
                      'value': self.v_linear,
                      'query': self.q_linear}

        self.softmax = nn.Softmax(dim=-1)
        self.dropout = nn.Dropout(dropout)
        self.out = nn.Linear(d_model, 128)

    def forward(self, entity_embedding):

        keys, values, queries = self.kvq_embedding(entity_embedding)

        # calculate attention using function we will define next
        scores = self.attention(queries, keys, values, self.d_k, self.dropout)

        # concatenate heads and put through final linear layer
        concat = scores.transpose(1, 2).contiguous().view(-1, self.d_model)
        output = self.out(concat)
        average_pooling = self.average_pooling(output)
        return average_pooling

    def kvq_embedding(self, entity_embedding):
        # batch size
        bs = entity_embedding.size(0)

        # derive key, query and value
        i = 0
        # keys = torch.zeros((bs, self.d_model), dtype=torch.float32, device=self.device)
        # values = torch.zeros((bs, self.d_model), dtype=torch.float32, device=self.device)
        # queries = torch.zeros((bs, self.d_model), dtype=torch.float32, device=self.device)

        keys = self.dense['key'](entity_embedding)
        values = self.dense['value'](entity_embedding)
        queries = self.dense['query'](entity_embedding)

        # perform linear operation and split into h heads
        keys = keys.view(-1, self.h, self.d_k)
        queries = queries.view(-1, self.h, self.d_k)
        values = values.view(-1, self.h, self.d_k)

        return keys, values, queries

    def masked(self):
        # generate masks from frontal vision cone and line of sight
        # shape (BS, T, NE)
        return None

    def average_pooling(self, input):
        '''
        input shape: [num_entities x num_feature]
        output shape: [num_feature]
        num_feature = d_model
        '''
        num_entity, num_feature = input.size()
        # masked
        self.visibility = torch.ones(num_entity, dtype=torch.float32, device=self.device)
        masked = self.visibility * input.transpose(1, 0)
        pooling_embedding = torch.sum(masked, dim=-2)
        denominator = torch.sum(self.visibility, dim=-1) + 1e-5
        output = pooling_embedding / denominator
        output = output.reshape(-1, 1)
        output = torch.mean(output, 0)
        return output

    def attention(self, query, key, value, d_k, dropout=None):
        scores = torch.matmul(query, key.transpose(-2, -1)) / math.sqrt(d_k)

        if self.mask is not None:
            mask = self.mask.unsqueeze(1)
            scores = scores.masked_fill(mask == 0, -1e9)

        scores = self.softmax(scores)

        if dropout is not None:
            scores = dropout(scores)

        output = torch.matmul(scores, value)
        return output



class PolicyArchitecture(nn.Module):
    def __init__(self, size):
        super().__init__()
        d_model = size
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

        self.multihead_attention = MultiHeadAttention(heads=2, d_model=d_model)
        self.dence = nn.Linear(1, 2)
       # self.sigmoid = nn.Sigmoid();
        if torch.cuda.is_available():
            self.multihead_attention.cuda(self.device)

        self.load_model()

    def forward(self, objects):
        object = torch.tensor(objects, dtype=torch.float32, device=self.device)
        pooled_object = self.multihead_attention.forward(object)
        result = self.dence(pooled_object)
        #result = self.sigmoid(pooled_object)
        return result

    def save_model(self, path = "Multi-Head Self-Attention.pth"):
        with data_lock:
            torch.save(self.state_dict(), path)

    def load_model(self, path = "Multi-Head Self-Attention.pth"):
        with data_lock:
            if os.path.isfile(path):
                self.load_state_dict(torch.load(path))
        #model.eval()