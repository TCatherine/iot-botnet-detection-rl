from agent import Agent, AgentSync
from communicator import Communicator
from env_stack import EnvStack

from threading import Thread
import mlflow
import time
from utils import Configuretion, plot_roc, add_metrics, init_mlflow
from experience_replay import ExperienceReplay


def main_loop(agent: Agent, envs, steps=501):
    fp, fn, tp, tn = 0, 0, 0, 0
    total_reward = 0
    local_reward = 0

    obs = envs.reset(agent.idx)

    obs = agent.fix_non_valid_state(obs)
    obs = agent.add_communication_to_state(obs)

    a = []
    warmup_epochs = 10
    for itr in range(steps):

        action, q_values = agent.select_action(obs)
        next_obs, reward, done, info = envs.step(agent.idx, action)

        next_obs = agent.fix_non_valid_state(next_obs)
        next_obs = agent.add_communication_to_state(next_obs)

        agent.em.add(obs, action, reward, next_obs)

        is_attack = [(int(i.split(' ')[1])) for i in info]
        a.append((itr, q_values, is_attack))
        total_reward += sum(reward)
        local_reward += sum(reward)

        if len(agent.em) > warmup_epochs:
            loss = agent.train()

        fp, fn, tp, tn = add_metrics(info, fp, fn, tp, tn)
        print(f'{fp=}, {fn=}, {tp=}, {tn=}')

        if len(agent.em) > warmup_epochs and itr % 1 == 0:

            print(f'agent_{agent.idx}, Step {itr + 1} Loss: {loss:0.3} Reward: {total_reward}')
            mlflow.log_metric(f"loss {agent.idx}", loss, step=itr)
            mlflow.log_metric(f"reward {agent.idx}", total_reward, step=itr)
            if len(agent.em) > warmup_epochs and itr % 20 == 0:
                agent.update_target_weights()

                mlflow.log_metric(f"local reward {agent.idx}", local_reward, step=itr)
                mlflow.log_metric(f"false positive {agent.idx}", fp, step=itr)
                mlflow.log_metric(f"false negative {agent.idx}", fn, step=itr)
                mlflow.log_metric(f"true positive {agent.idx}", tp, step=itr)
                mlflow.log_metric(f"true negative {agent.idx}", tn, step=itr)
                # agent.policy.save_model(agent.policy.path)
                fp, fn, tp, tn = 0, 0, 0, 0
                local_reward = 0

        agent.eps = agent.eps - agent.eps_delta
        obs = next_obs

        if itr % 10 == 0 and itr > 0:
            plot_roc(a, itr, agent.idx)

    envs.close(agent.idx)


def single_thread_pipeline(agent, envs):
    envs.ini_simulation(agent.idx)
    main_loop(agent, envs)


def get_agents(conf: Configuretion, comm, em):
    if conf.is_enable_communication:
        agents = [AgentSync(idx=idx,
                            num_features=conf.n_features,
                            num_envs=conf.num_env,
                            is_feature=conf.is_feature,
                            comm=comm,
                            em=em
                            ) for idx in range(conf.num_agent)]
    else:
        agents = [Agent(idx=idx,
                        num_features=conf.n_features,
                        num_envs=conf.num_env,
                        em=em
                        ) for idx in range(conf.num_agent)]

    return agents


if __name__ == "__main__":
    config_path = "../config.json"
    conf = Configuretion(config_path)
    comm = Communicator(conf.num_agent)

    if conf.is_enable_communication:
        em = ExperienceReplay('experience_sync.pickle')
    else:
        em = ExperienceReplay('experience.pickle')

    agents = get_agents(conf, comm, em)
    envs = EnvStack(conf)
    time.sleep(20)

    init_mlflow(conf.experiment_name)
    # with mlflow.start_run():
    list_threads = [Thread(target=single_thread_pipeline, args=(agents[idx], envs))
                    for idx in range(conf.num_agent)]
    for thread in list_threads:
        thread.start()

    [thread.join() for thread in list_threads]
