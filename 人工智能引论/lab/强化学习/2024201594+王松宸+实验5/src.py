import numpy as np
np.random.seed(0)

## 两个TODO均在本代码文件中，逐个实现时可注释掉无关代码。

# 定义状态转移概率矩阵P
P = [
    [0.9, 0.1, 0.0, 0.0, 0.0, 0.0],
    [0.5, 0.0, 0.5, 0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0, 0.6, 0.0, 0.4],
    [0.0, 0.0, 0.0, 0.0, 0.3, 0.7],
    [0.0, 0.2, 0.3, 0.5, 0.0, 0.0],
    [0.0, 0.0, 0.0, 0.0, 0.0, 1.0],
]
P = np.array(P)

rewards = [-1, -2, -2, 10, 1, 0]  # 定义奖励函数
gamma = 0.5  # 定义折扣因子


# 给定一条序列,计算从某个索引（起始状态）开始到序列最后（终止状态）得到的回报
def compute_return(start_index, chain, gamma):
    G = 0
    for i in reversed(range(start_index, len(chain))):
        # TODO ~1: 实现回报函数
        G += rewards[chain[i]-1] * (gamma**(i-start_index))
    return G


# 一个状态序列,s1-s2-s3-s6
chain = [1, 2, 3, 6]
start_index = 0
G = compute_return(start_index, chain, gamma)
print("根据本序列计算得到回报为：%s。" % G)




def compute(P, rewards, gamma, states_num):
    value=0
    ''' 利用贝尔曼方程的矩阵形式计算解析解,states_num是MRP的状态数 '''
    rewards = np.array(rewards).reshape((-1, 1))  # 将rewards写成列向量形式
    value = np.dot(np.linalg.inv(np.eye(states_num, states_num) - gamma * P),
                   rewards)
    return value


V = compute(P, rewards, gamma, 6)
print("MRP中每个状态价值分别为\n", V)

S = ["s1", "s2", "s3", "s4", "s5"]  # 状态集合
A = ["保持s1", "前往s1", "前往s2", "前往s3", "前往s4", "前往s5", "概率前往"]  # 动作集合
# 状态转移函数
P = {
    "s1-保持s1-s1": 1.0,
    "s1-前往s2-s2": 1.0,
    "s2-前往s1-s1": 1.0,
    "s2-前往s3-s3": 1.0,
    "s3-前往s4-s4": 1.0,
    "s3-前往s5-s5": 1.0,
    "s4-前往s5-s5": 1.0,
    "s4-概率前往-s2": 0.8,
    "s4-概率前往-s3": 0.1,
    "s4-概率前往-s4": 0.1,
}
# 奖励函数
R = {
    "s1-保持s1": -1,
    "s1-前往s2": 0,
    "s2-前往s1": -1,
    "s2-前往s3": -2,
    "s3-前往s4": -2,
    "s3-前往s5": 0,
    "s4-前往s5": 10,
    "s4-概率前往": 1,
}
gamma = 0.5  # 折扣因子
MDP = (S, A, P, R, gamma)

# 策略1,随机策略
Pi_1 = {
    "s1-保持s1": 0.5,
    "s1-前往s2": 0.5,
    "s2-前往s1": 0.5,
    "s2-前往s3": 0.5,
    "s3-前往s4": 0.5,
    "s3-前往s5": 0.5,
    "s4-前往s5": 0.5,
    "s4-概率前往": 0.5,
}
# 策略2
Pi_2 = {
    "s1-保持s1": 0.6,
    "s1-前往s2": 0.4,
    "s2-前往s1": 0.3,
    "s2-前往s3": 0.7,
    "s3-前往s4": 0.5,
    "s3-前往s5": 0.5,
    "s4-前往s5": 0.1,
    "s4-概率前往": 0.9,
}


# 把输入的两个字符串通过“-”连接,便于使用上述定义的P、R变量
def join(str1, str2):
    return str1 + '-' + str2

# 根据给定的 MDP (P_dict, R_dict) 与策略 Pi，将其转换为 MRP 的 (P^pi, R^pi)
def build_mrp_from_mdp_policy(S, P_dict, R_dict, Pi):
    n = len(S)
    idx = {s: i for i, s in enumerate(S)}
    P_mrp = np.zeros((n, n))
    R_mrp = np.zeros(n)

    # 将策略按状态聚合，Pi 的键为 "state-action"
    pi_by_state = {}
    for key, prob in Pi.items():
        if prob == 0:
            continue
        # 形如 "s1-保持s1"
        parts = key.split('-', 1)
        if len(parts) != 2:
            continue
        s, a = parts
        pi_by_state.setdefault(s, []).append((a, prob))

    for s in S:
        i = idx[s]
        actions = pi_by_state.get(s, [])
        if not actions:
            # 若该状态无可用动作（如吸收态），按吸收态处理
            P_mrp[i, i] = 1.0
            R_mrp[i] = 0.0
            continue

        # 期望即时奖励 R^pi(s) = sum_a pi(a|s) R(s,a)
        r_exp = 0.0
        for a, prob in actions:
            r_key = f"{s}-{a}"
            if r_key in R_dict:
                r_exp += prob * R_dict[r_key]
        R_mrp[i] = r_exp

        # 转移概率 P^pi(s,s') = sum_a pi(a|s) P(s,a,s')
        for a, prob in actions:
            for sp in S:
                j = idx[sp]
                p_key = f"{s}-{a}-{sp}"
                if p_key in P_dict:
                    P_mrp[i, j] += prob * P_dict[p_key]

        # 若数值误差导致行和略偏离 1，可进行归一化（避免硬性修改，这里仅在接近 0 时兜底）
        row_sum = P_mrp[i].sum()
        if row_sum == 0:
            P_mrp[i, i] = 1.0

    return P_mrp, R_mrp

# TODO ~2: 动手修改状态转移函数等参数，体会决策过程变化
gamma = 0.5
# 由 MDP + 策略得到对应的 MRP（默认展示策略 Pi_1 与 Pi_2 的对比）
P_from_mdp_to_mrp_1, R_from_mdp_to_mrp_1 = build_mrp_from_mdp_policy(S, P, R, Pi_1)
V = compute(P_from_mdp_to_mrp_1, R_from_mdp_to_mrp_1, gamma, len(S))
print("MDP(按策略 Pi_1) 中每个状态价值分别为\n", V)

P_from_mdp_to_mrp_2, R_from_mdp_to_mrp_2 = build_mrp_from_mdp_policy(S, P, R, Pi_2)
V2 = compute(P_from_mdp_to_mrp_2, R_from_mdp_to_mrp_2, gamma, len(S))
print("MDP(按策略 Pi_2) 中每个状态价值分别为\n", V2)
