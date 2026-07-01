"""
Introduction to Machine Learning

Lab 10: Primal SVM and Suppress Unfairness

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab10.py on Python >=3.6;
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
    df['income'] = df['income'].replace('<=50K', -1)
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


def hinge_loss_with_grad(z: np.ndarray, y: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    The hinge loss L = max(0, 1-yz) and its gradient dL/dz
    :param z: an array with arbitrary size
    :param y: an array having the same size with z
    :return:
        the output of the hinge loss and its gradient
    """
    # Hinge loss: L = max(0, 1 - y*z)
    margin = 1 - y * z
    # 平均损失值（标量）
    loss = np.mean(np.maximum(0, margin))
    # 梯度 dL/dz: margin > 0 时为 -y，否则为 0（逐样本梯度）
    grad = np.where(margin > 0, -y, 0).astype(np.float64)
    return loss, grad


def linear_model_with_grad(xs: np.ndarray, weights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    The linear model: y = x^T w and its gradient dy/dw
    :param xs: the data with size (N, D), where N is the number of sample, D is the dimension of feature
    :param weights: the parameters of linear model with size (D, 1)
    :return:
        the output of the model with size (N, 1)
        the gradient of the model with size (N, D)
    """
    # 线性模型前向：z = x @ w，输出大小 (N, 1)
    z = xs @ weights
    # 梯度 dz/dw = x，大小 (N, D)
    grad = xs.copy()
    return z, grad


def sgd_primal_svm(xs: np.ndarray, ys: np.ndarray, batch_size: int = 100,
                   epochs: int = 50, lr: float = 1e-1) -> np.ndarray:
    """
    Training a Logistic regression model based on stochastic gradient descent
    :param xs: training data with size (N, D)
    :param ys: training labels with size (N, 1)
    :param batch_size: the batch size of SGD
    :param epochs: the number of epochs
    :param lr: the learning rate
    :return:
        the model parameters with size (D + 1, 1)
    """
    num, dim = xs.shape
    # 拼接 bias 项: x -> [x, -1]，权重维度变为 (D+1, 1)
    xs = np.concatenate((xs, -np.ones((xs.shape[0], 1))), axis=1)
    weights = np.random.RandomState(1).randn(dim + 1, 1)
    # SGD 训练 SVM（使用 hinge loss）
    for epoch in range(epochs):
        # 每轮随机打乱数据
        idx = np.random.permutation(num)
        xs_shuffled = xs[idx]
        ys_shuffled = ys[idx]
        for i in range(0, num, batch_size):
            # 取一个 mini-batch
            x_batch = xs_shuffled[i:i + batch_size]
            y_batch = ys_shuffled[i:i + batch_size]
            # 前向传播：计算 z = x @ w 及 dz/dw
            z, _ = linear_model_with_grad(x_batch, weights)
            # 计算 hinge loss 及 dL/dz（逐样本梯度）
            _, dL_dz = hinge_loss_with_grad(z, y_batch)
            # 链式法则: dL/dw = (1/B) * x^T @ dL_dz
            grad_w = x_batch.T @ dL_dz / x_batch.shape[0]
            # SGD 更新
            weights -= lr * grad_w

    return weights # 打乱顺序，分batch，算梯度，更新权重


def test_svm(xs: np.ndarray, ys: np.ndarray, weights: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    Get prediction accuracy of the logistic regression model
    :param xs: testing data with size (N, D)
    :param ys: the ground truth labels with size (N, 1)
    :param weights: the model parameters with size (D, 1)
    :return:
        prediction accuracy in the range in [0, 1]
        prediction results with size (N, 1)
    """
    # 同样拼接 bias 项
    xs = np.concatenate((xs, -np.ones((xs.shape[0], 1))), axis=1)
    # 计算 z = x @ w
    z = xs @ weights
    # SVM 预测: sign(z) => z>=0 预测为 +1, z<0 预测为 -1
    preds = np.where(z >= 0, 1, -1)
    acc = np.mean(preds == ys)
    return acc, preds


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


def data_augment(data: List) -> List:
    """
    Find a way to augment data for training a model with better fairness on gender
    :param data: a list [samples, labels, genders]
    :return:
        augmented data: a list [samples, labels]
    """
    xs, ys, genders = data
    ys = ys.ravel()
    # 将数据按 (性别, 标签) 分为 4 组
    male_pos = (genders == 1) & (ys == 1)      # 男性高收入
    male_neg = (genders == 1) & (ys == -1)     # 男性低收入
    female_pos = (genders == 0) & (ys == 1)    # 女性高收入
    female_neg = (genders == 0) & (ys == -1)   # 女性低收入
    groups = [male_pos, male_neg, female_pos, female_neg]
    # 上采样：将所有组扩充到相同大小，消除性别与标签的分布偏差
    target = max(m.sum() for m in groups)
    xs_list, ys_list = [], []
    for mask in groups:
        idx = np.where(mask)[0]
        idx_resampled = np.random.choice(idx, size=target, replace=True)
        xs_list.append(xs[idx_resampled])
        ys_list.append(ys[idx_resampled].reshape(-1, 1))
    xs_new = np.vstack(xs_list)
    ys_new = np.vstack(ys_list)
    return [xs_new, ys_new]


if __name__ == '__main__':
    data = adult_income_data_loader()
    weights1 = sgd_primal_svm(xs=data['train'][0], ys=data['train'][1])
    accuracy1, preds1 = test_svm(xs=data['test'][0], ys=data['test'][1], weights=weights1)
    p1, p0 = gender_fairness_check(preds1[:, 0], genders=data['test'][2])
    print('SVM: p(high income | male)={:.4f}, p(high income | female)={:.4f}'.format(p1, p0))
    print('SVM: Acc={:.4f}'.format(accuracy1))

    data_new = data_augment(data['train'])
    weights2 = sgd_primal_svm(xs=data_new[0], ys=data_new[1])
    accuracy2, preds2 = test_svm(xs=data['test'][0], ys=data['test'][1], weights=weights2)
    q1, q0 = gender_fairness_check(preds2[:, 0], genders=data['test'][2])
    print('After DA, SVM: p(high income | male)={:.4f}, p(high income | female)={:.4f}'.format(q1, q0))
    print('After DA, SVM: Acc={:.4f}'.format(accuracy2))
