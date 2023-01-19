import json

import mlflow
import numpy as np
import pandas as pd
import plotly.express as px
from sklearn import metrics


class Configuretion:
    def __init__(self, config_path):
        config_file = open(config_path, "r")
        json_string = json.load(config_file)["GeneralInfo"]
        self.num_agent = json_string["Number Agents"]
        self.num_env = json_string["Number Env"]
        self.port = json_string["First Port"]
        self.n_features = json_string["Number Features"]
        self.is_enable_communication = json_string["IsEnableCommunication"]
        self.is_feature = json_string["IsFeature"]
        path_list = json_string["WeightPath"]
        idx = ((2 if self.is_feature else 1) if self.is_enable_communication else 0)
        self.weight_path = path_list[idx]
        self.experiment_name = f'Agents'
        self.experiment_name += (
            f'-with-communicat{1 if self.is_feature else 0}' if self.is_enable_communication else '')
        self.max_number_flows = json_string['max_number_flows']


def add_metrics(string, fp, fn, tp, tn):
    for str in string:
        words = str.split(' ')
        fp += int(words[3])
        fn += int(words[5])
        tp += int(words[7])
        tn += int(words[9])
    return fp, fn, tp, tn


def init_mlflow(experiment_name):
    mlflow.set_tracking_uri('mlruns/')
    try:
        mlflow.create_experiment(experiment_name)
    except:
        print(f'Experiment {experiment_name} already exist')
    mlflow.set_experiment(experiment_name)


def plot_roc(a, step, idx):
    a = a[5:]

    it, qv, gt = list(zip(*a))

    qv_np = np.array(qv)
    qv_np = qv_np[1:, 0, 1]

    gt_np = np.array(gt)
    gt_np = gt_np[:-1, 0]

    fpr, tpr, _ = metrics.roc_curve(gt_np, qv_np)

    fig = px.area(
        x=fpr, y=tpr,
        title=f'ROC Curve (AUC={metrics.auc(fpr, tpr):.4f})',
        labels=dict(x='False Positive Rate', y='True Positive Rate'),
        width=700, height=500
    )
    # fig.show()

    # fig.write_html(f'roc_step-{step}__agent-{idx}.html')

    roc = pd.DataFrame({'tpr': tpr, 'fpr': fpr})
    roc.to_csv(f'roc_step-{step}__agent-{idx}.csv')

    mlflow.log_figure(fig, f'roc_plots/roc_step-{step}__agent-{idx}.html')
    mlflow.log_artifact(f'roc_step-{step}__agent-{idx}.csv')
