import pandas as pd
import numpy as np
import torch
import policy
import mlflow

def init_mlflow(experiment_name):
    mlflow.set_tracking_uri('mlruns/')
    try:
        mlflow.create_experiment(experiment_name)
    except:
        print(f'Experiment {experiment_name} already exist')
    mlflow.set_experiment(experiment_name)

def loop(data, res):
    res = np.array([np.array(x) for x in res])
    learning_rate = 0.00003
    loss = torch.nn.CrossEntropyLoss(reduction='mean')
    max_number_flows = 20
    number_features = 5
    net = policy.SimplePolicyArchitecture(number_features, max_number_flows, 'simple_network.pth')
    optimizer = torch.optim.Adam(net.parameters(), lr=learning_rate)

    # true positive, true negative
    true_res = [0, 0]
    # false positive, true negative
    false_res = [0, 0]

    for _ in range(50):
        # true positive, true negative
        true_res = [0, 0]
        # false positive, true negative
        false_res = [0, 0]
        for d, r in zip(data, res):

            d = d.tolist()
            k = max_number_flows - len(d)
            d.extend([[0 for _ in range(number_features)] for _ in range(k)])

            d = torch.tensor(d, dtype=torch.float32).reshape(-1, max_number_flows, number_features)
            d = torch.transpose(d, 1, 2)
            Q_value = net.forward(d)

            res_loss = loss(Q_value, torch.max(torch.tensor([r]), 1)[1])
            res_loss.backward()
            optimizer.step()

            idx = 0
            if Q_value[0][0] < Q_value[0][1]:
                idx=1

            if r[idx]==1:
                true_res[idx]+=1
            else:
                false_res[idx]+=1
            # print(res_loss.detach().tolist(), Q_value.detach().tolist(), r)

            mlflow.log_metric(f"loss", res_loss.detach().tolist())
        mlflow.log_metric(f"tp", true_res[0])
        mlflow.log_metric(f"tn", true_res[1])
        mlflow.log_metric(f"fp", false_res[0])
        mlflow.log_metric(f"fn", false_res[1])
        print(f'true_res(tp, tn) = {true_res}', f'false_res(fp, fn) = {false_res}')
        print(f'accuarcy = {sum(true_res)/(sum(true_res)+sum(false_res))}')

def parse_file_with_union(file_name):
    fixed_df = pd.read_csv(file_name, sep=',')
    idx = fixed_df.loc[:, '№'].values
    values_name = ['pack_num', 'pack_size', 'duration', 'interval', 'deviation_interval']
    data = []
    res = []

    prev_idx = -1
    start_idx = 0

    for i, num in enumerate(idx):
        if num > prev_idx:
            prev_idx = num
            continue
        target = [0, 0]
        is_attack = fixed_df.loc[start_idx:start_idx, 'is_attack'].values
        target[int(is_attack)] = 1

        flows = fixed_df.loc[start_idx:i-1, values_name].values
        data.append(flows)

        res.append(target)
        prev_idx = num
        start_idx = i

    return data, res

if __name__ == "__main__":
    path = "../ns-3.29/"
    num_agent = 3
    num_env = 3
    path_list = []
    init_mlflow('simply network')
    for a in range(num_agent):
        for e in range(num_env):
            path_list.append(f'{path}dataset{e}_{a}.csv')

    batch = []
    res = []
    for path in path_list:
        d, r = parse_file_with_union(path)
        batch.extend(d)
        res.extend(r)

    loop(batch, res)