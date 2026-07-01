"""
Introduction to Machine Learning

Lab 9: LDA and Logistic Regression

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab9.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s):王松宸
Student ID(s):2024201594
"""

import pandas as pd
import numpy as np
from typing import Tuple, Dict, List
# don't add any other packages


def adult_income_data_loader() -> Dict[str, List[np.ndarray]]:
    df = pd.read_csv("adult.csv")
    df.drop(df.index[df['workclass'] == '?'], inplace=True)
    df.drop(df.index[df['occupation'] == '?'], inplace=True)
    df.drop(df.index[df['native-country'] == '?'], inplace=True)
    df.dropna(how='any', inplace=True)
    df = df.drop_duplicates()
    df.drop(['education'], axis=1, inplace=True)
    df['net_capital'] = (df['capital-gain'] - df['capital-loss']).astype(int)
    df.drop(['capital-gain', 'capital-loss'], axis=1, inplace=True)
    # changing class from >50K and <=50K to 1 and 0
    df['income'] = df['income'].astype(str)
    df['income'] = df['income'].replace('>50K', 1)
    df['income'] = df['income'].replace('<=50K', 0)
    # changing class from Male and Female to 1 and 0
    df['gender'] = df['gender'].astype(str)
    df['gender'] = df['gender'].replace('Male', 1)
    df['gender'] = df['gender'].replace('Female', 0)
    b = df.iloc[:, [0, 2, 3, 9, 12]]
    ys = df['income'].to_numpy()
    ys = ys.reshape(ys.shape[0], 1)
    genders = df['gender'].to_numpy()
    names = b.columns
    xs = pd.DataFrame(b, columns=names).to_numpy()
    xs = np.float64(xs)
    # normalize features
    xs /= np.max(xs, axis=0, keepdims=True)
    idx = np.random.RandomState(42).permutation(xs.shape[0])
    data = {'train': [xs[idx[:10000], :], ys[idx[:10000], :], genders[idx[:10000]]],
            'test': [xs[idx[10000:20000], :], ys[idx[10000:20000], :], genders[idx[10000:20000]]]}
    return data


def linear_discriminant_analysis_2class(xs: np.ndarray, ys: np.ndarray) -> Tuple[np.ndarray, float]:
    """
    Learning a LDA model for two classes: learning w and c for checking x^T w > c or not
    :param xs: training data with size (N, D)
    :param ys: training labels with size (N, 1), whose element is 0 or 1

    :return:
        the weights "w" of LDA with size (D, 1),
        the criterion "c"
    """
    # 将标签展平为一维，方便布尔索引
    y = ys[:, 0]

    # 按类别分离数据
    X0 = xs[y == 0]  # (N0, D) — 低收入样本
    X1 = xs[y == 1]  # (N1, D) — 高收入样本
    N0, N1 = X0.shape[0], X1.shape[0]

    # 计算每类的均值向量 (D, 1)
    mu0 = X0.mean(axis=0, keepdims=True).T  # (D, 1)
    mu1 = X1.mean(axis=0, keepdims=True).T  # (D, 1)

    # 计算类内散度矩阵 (D, D)
    # S_k = sum_i (x_i - mu_k)(x_i - mu_k)^T
    diff0 = X0 - mu0.T  # (N0, D), 每行是 x_i - mu0
    diff1 = X1 - mu1.T  # (N1, D), 每行是 x_i - mu1
    S0 = diff0.T @ diff0  # (D, D)
    S1 = diff1.T @ diff1  # (D, D)

    # 共享协方差矩阵 + 逆；用伪逆防止奇异矩阵
    Sigma = (S0 + S1) / (N0 + N1 - 2)  # pooled covariance
    Sigma_inv = np.linalg.pinv(Sigma)   # (D, D) 伪逆

    # 计算 w 和 c
    w = Sigma_inv @ (mu1 - mu0)         # (D, 1)
    c = 0.5 * (mu1 + mu0).T @ w + np.log(N0 / N1)  # 标量
    c = c.item()  # 提取 Python float

    return w, c


def test_lda(xs: np.ndarray, ys: np.ndarray, w: np.ndarray, c: float) -> Tuple[np.ndarray, np.ndarray]:
    """
    Testing the LDA model and output prediction results and accuracy
    :param xs: testing data with size (N, D)
    :param ys: the ground truth labels with size (N, 1)
    :param w: the model parameters with size (D, 1)
    :param c: the threshold to make classification x^Tw > c => 1, otherwise => 0
    :return:
        prediction accuracy in the range in [0, 1]
        prediction results with size (N, 1)
    """
    # 对每个样本计算 x^T w，与阈值 c 比较
    scores = xs @ w  # (N, 1)
    preds = (scores > c).astype(np.float64)  # >c → 1, ≤c → 0
    accuracy = np.mean(preds == ys)  # 准确率
    return accuracy, preds


def sigmoid_function_with_grad(x: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    The sigmoid function y = 1 / (1 + exp(-x)) and its gradient
    :param x: an array with arbitrary size
    :return:
        the output of the sigmoid function
        the gradient dy/dx
    """
    # sigmoid: y = 1 / (1 + exp(-x))
    # 为防止 exp 溢出，将 x 裁剪到一个安全范围
    x_safe = np.clip(x, -500, 500)
    y = 1.0 / (1.0 + np.exp(-x_safe))
    # 梯度: dy/dx = y * (1 - y) 
    grad = y * (1.0 - y)
    return y, grad


def binary_cross_entropy_with_grad(ps: np.ndarray, ys: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    The BCE loss:
        L = -1/N * sum_n yn * log pn + (1-yn) * log (1-pn)
    And its gradient dL/dp
    :param ps: the probabilities of labels
    :param ys: the binary labels
    :return:
        the value of loss function
        th gradient dL/dp, whose size is the same with ps
    """
    N = ps.shape[0]  # 样本数
    # 裁剪概率避免 log(0) 的数值问题
    eps = 1e-12
    p = np.clip(ps, eps, 1.0 - eps)
    # BCE: L = -(1/N) * sum( y*log(p) + (1-y)*log(1-p) )
    loss = -np.mean(ys * np.log(p) + (1.0 - ys) * np.log(1.0 - p))
    # 梯度: dL/dp = -(1/N) * (y/p - (1-y)/(1-p))
    grad = -(ys / p - (1.0 - ys) / (1.0 - p)) / N
    return np.array([loss]), grad


def linear_model_with_grad(xs: np.ndarray, weights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    The linear model: y = x^T w and its gradient dy/dw
    :param xs: the data with size (N, D), where N is the number of sample, D is the dimension of feature
    :param weights: the parameters of linear model with size (D, 1)
    :return:
        the output of the model with size (N, 1)
        the gradient of the model with size (N, D)
    """
    # 线性映射: y = x @ w, 输出 (N, 1)
    y = xs @ weights
    # dy_i/dw_j = x_{ij}, 所以梯度矩阵就是 xs 本身 (N, D)
    grad = xs
    return y, grad


def sgd_logistic_regression(xs: np.ndarray, ys: np.ndarray, batch_size: int = 100,
                            epochs: int = 50, lr: float = 1e-1) -> np.ndarray:
    """
    Training a Logistic regression model based on stochastic gradient descent
    :param xs: training data with size (N, D)
    :param ys: training labels with size (N, 1)
    :param batch_size: the batch size of SGD
    :param epochs: the number of epochs
    :param lr: the learning rate
    :return:
        the model parameters with size (D, 1)
    """
    num, dim = xs.shape
    weights = np.random.RandomState(1).randn(dim, 1)

    # SGD 主循环
    for epoch in range(epochs):
        # 每个 epoch 随机打乱数据
        idx = np.random.permutation(num)
        xs_shuffled = xs[idx]
        ys_shuffled = ys[idx]

        # 按 batch_size 遍历全部数据
        for start in range(0, num, batch_size):
            end = min(start + batch_size, num)
            x_batch = xs_shuffled[start:end]  # (B, D)
            y_batch = ys_shuffled[start:end]  # (B, 1)

            # ---- 前向传播 ----
            # 线性映射
            y_linear, _ = linear_model_with_grad(x_batch, weights)  # (B, 1)
            # sigmoid 激活
            p, d_sigmoid = sigmoid_function_with_grad(y_linear)     # (B, 1)
            # BCE 损失
            loss, d_bce = binary_cross_entropy_with_grad(p, y_batch)

            # ---- 反向传播（链式法则）----
            # dL/dy = dL/dp * dp/dy  （逐元素相乘）
            dL_dy = d_bce * d_sigmoid  # (B, 1)
            # dL/dw = x^T @ dL/dy   （因为 dy/dw = x）
            grad = x_batch.T @ dL_dy  # (D, 1)

            # ---- 参数更新 ----
            weights -= lr * grad

    return weights


def test_logistic_regression(xs: np.ndarray, ys: np.ndarray, weights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    Get prediction accuracy of the logistic regression model
    :param xs: testing data with size (N, D)
    :param ys: the ground truth labels with size (N, 1)
    :param weights: the model parameters with size (D, 1)
    :return:
        prediction accuracy in the range in [0, 1]
        prediction results with size (N, 1)
    """
    # 前向传播：线性映射 + sigmoid 得到概率
    y_linear, _ = linear_model_with_grad(xs, weights)  # (N, 1)
    p, _ = sigmoid_function_with_grad(y_linear)         # (N, 1), 概率值
    # 概率 > 0.5 预测为 1，否则 0
    preds = (p > 0.5).astype(np.float64)
    accuracy = np.mean(preds == ys)
    return accuracy, preds


def gender_fairness_check(preds: np.ndarray, genders: np.ndarray) -> Tuple[float, float]:
    """
    Find a way to check whether your classification results are fair with respect to gender or not
    :param preds: the results with size (N, )
    :param genders: the gender info with size (N, ), 1 for male and 0 for female
    :return:
        p(y=1|male) and p(y=1|female)
    """
    p1 = preds[genders == 1]
    p0 = preds[genders == 0]
    return np.sum(p1 == 1) / p1.shape[0], np.sum(p0 == 1) / p0.shape[0]


if __name__ == '__main__':
    data = adult_income_data_loader()
    weights = sgd_logistic_regression(xs=data['train'][0], ys=data['train'][1])
    accuracy1, preds1 = test_logistic_regression(xs=data['test'][0], ys=data['test'][1], weights=weights)

    w, c = linear_discriminant_analysis_2class(xs=data['train'][0], ys=data['train'][1])
    accuracy2, preds2 = test_lda(xs=data['test'][0], ys=data['test'][1], w=w, c=c)

    print('LR: Acc={:.4f}'.format(accuracy1))
    print('LDA: Acc={:.4f}'.format(accuracy2))

    p1, p0 = gender_fairness_check(preds1[:, 0], genders=data['test'][2])
    q1, q0 = gender_fairness_check(preds2[:, 0], genders=data['test'][2])

    print('LR: p(high income | male)={:.4f}, p(high income | female)={:.4f}'.format(p1, p0))
    print('LDA: p(high income | male)={:.4f}, p(high income | female)={:.4f}'.format(q1, q0))

    # ---- 公平性度量 ----
    # 1. Demographic Parity Difference (人口统计均等差异)
    #    衡量不同群体被预测为正类的概率差异，越小越公平
    dpd_lr = abs(p1 - p0)
    dpd_lda = abs(q1 - q0)
    print('\n--- Fairness Metrics ---')
    print('Demographic Parity Difference (越小越公平):')
    print('  LR : |P(high|male) - P(high|female)| = {:.4f}'.format(dpd_lr))
    print('  LDA: |P(high|male) - P(high|female)| = {:.4f}'.format(dpd_lda))

    # 2. Equal Opportunity Difference (机会均等差异)
    #    衡量真正的高收入者在不同性别中被正确识别的概率差异
    ys_test = data['test'][1][:, 0]
    genders_test = data['test'][2]

    # 找出真实标签为1的样本中，按性别分别计算 TPR (True Positive Rate)
    tpr_male_lr = np.mean(preds1[(genders_test == 1) & (ys_test == 1), 0] == 1)
    tpr_female_lr = np.mean(preds1[(genders_test == 0) & (ys_test == 1), 0] == 1)
    tpr_male_lda = np.mean(preds2[(genders_test == 1) & (ys_test == 1), 0] == 1)
    tpr_female_lda = np.mean(preds2[(genders_test == 0) & (ys_test == 1), 0] == 1)

    eod_lr = abs(tpr_male_lr - tpr_female_lr)
    eod_lda = abs(tpr_male_lda - tpr_female_lda)
    print('Equal Opportunity Difference (越小越公平):')
    print('  LR : |TPR_male - TPR_female| = {:.4f}'.format(eod_lr))
    print('  LDA: |TPR_male - TPR_female| = {:.4f}'.format(eod_lda))

    # 综合结论
    print('Overall: 两个指标越接近0表示分类器对不同性别越公平')
