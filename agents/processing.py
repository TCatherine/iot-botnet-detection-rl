import pandas as pd
from sklearn.decomposition import PCA
import matplotlib.pyplot as plt
from sklearn.manifold import TSNE, MDS


def add_point(ax, finalDf):
    targets = [0, 1]
    colors = ['r', 'g']
    for target, color in zip(targets, colors):
        indicesToKeep = finalDf['is_attack'] == target
        ax.scatter(finalDf.loc[indicesToKeep, 'principal component 1']
                   , finalDf.loc[indicesToKeep, 'principal component 2']
                   , c=color
                   , s=50)


def pca_graph():
    fig = plt.figure(figsize=(8, 8))
    ax = fig.add_subplot(1, 1, 1)
    return ax


def parse_file_with_union(file_name):
    fixed_df = pd.read_csv(file_name, sep=',')
    idx = fixed_df.loc[:, '№'].values
    values_name = ['pack_num', 'pack_size', 'duration', 'interval', 'deviation_interval']
    new_Df = pd.DataFrame(columns=values_name)
    res_Df = pd.DataFrame(columns=['is_attack'])

    prev_idx = -1
    start_idx = 0

    for i, num in enumerate(idx):
        if num > prev_idx:
            prev_idx = num
            continue
        res = fixed_df.loc[start_idx:start_idx, 'is_attack'].to_frame()
        flows = fixed_df.loc[start_idx:i - 1, values_name]
        ins = flows.mean().to_frame().T
        new_Df = pd.concat([new_Df, ins], axis=0)
        res_Df = pd.concat([res_Df, res], axis=0)
        prev_idx = num
        start_idx = i

    new_Df.reset_index(inplace=True, drop=True)
    res_Df.reset_index(inplace=True, drop=True)

    return new_Df.values, res_Df


def parse_file(file_name):
    fixed_df = pd.read_csv(file_name, sep=',')

    values_name = ['pack_num', 'pack_size', 'duration', 'interval', 'deviation_interval']
    data = fixed_df.loc[:, values_name].values
    return data, fixed_df[['is_attack']]


def get_tSNE(data, res):
    tsne = TSNE(n_components=2, init='random')
    principalComponents = tsne.fit_transform(data)
    principalDf = pd.DataFrame(data=principalComponents
                               , columns=['principal component 1', 'principal component 2'])
    finalDf = pd.concat([principalDf, res], axis=1)
    add_point(ax, finalDf)


def get_PCA(data, res):
    pca = PCA(n_components=2)
    principalComponents = pca.fit_transform(data)
    principalDf = pd.DataFrame(data=principalComponents
                               , columns=['principal component 1', 'principal component 2'])
    finalDf = pd.concat([principalDf, res], axis=1)
    add_point(ax, finalDf)


def get_MDS(data, res):
    mds = MDS(n_components=2)
    principalComponents = mds.fit_transform(data)
    principalDf = pd.DataFrame(data=principalComponents
                               , columns=['principal component 1', 'principal component 2'])
    finalDf = pd.concat([principalDf, res], axis=1)
    add_point(ax, finalDf)


if __name__ == "__main__":
    path = "../ns-3.29/"
    num_agent = 3
    num_env = 3
    path_list = []
    for a in range(num_agent):
        for e in range(num_env):
            path_list.append(f'{path}dataset{e}_{a}.csv')

    ax = pca_graph()
    for path in path_list:
        data, res = parse_file_with_union(path)
        get_PCA(data, res)

    targets = ['clear', 'attack']
    ax.legend(targets)
    ax.grid()
    plt.show()
    a = 1
