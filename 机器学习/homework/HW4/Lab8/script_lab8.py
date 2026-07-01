"""
Introduction to Machine Learning

Lab 8: Mean-shift and label propagation

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab8.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s):王松宸
Student ID(s):2024201594
"""

import copy
import matplotlib.pyplot as plt
import numpy as np
from typing import Tuple

# don't add any other packages


def visualization_pts(pts: np.ndarray, label: str, colors: np.ndarray = None):
    if colors is None:
        plt.scatter(pts[:, 0], pts[:, 1], label=label)
    else:
        if colors.shape[0] == pts.shape[0]:
            for n in range(pts.shape[0]):
                plt.scatter(pts[n, 0], pts[n, 1], color=colors[n, :])
        else:
            plt.scatter(pts[:, 0], pts[:, 1], label=label, color=colors)


def p_distance(x: np.ndarray, y: np.ndarray, p: int = 2):
    return np.sum(
        np.abs(np.expand_dims(x, axis=1) - np.expand_dims(y, axis=0)) ** p, axis=2
    )


def kernel(
    x: np.ndarray, y: np.ndarray = None, k_type: str = "rbf", bandwidth: float = 0.1
) -> np.ndarray:
    """
    Implement four kinds of typical kernel functions
    1) RBF kernel: k(x, y) = exp(-||x - y||_2^2 / bandwidth)
    2) 'Gate' kernel: k(x, y) = 1/bandwidth if ||x - y||_1 <= bandwidth, = 0 otherwise
    3) 'Triangle' kernel: k(x, y) = (bandwidth - ||x - y||_1) if ||x - y||_1 <= bandwidth, = 0 otherwise
    4) Linear kernel: k(x, y) = <x, y>
    :param x: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param y: a set of samples with size (M, D), where M is the number of samples. this input can be None
    :param k_type: the type of kernels, including 'rbf', 'gate', 'triangle', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf/gate/triangle kernels
    :return:
        if y = None, return a matrix with size (N, N)
        otherwise, return a matrix with size (M, N)
    """
    if y is None:
        y = copy.deepcopy(x)
    if k_type == "rbf":
        dist = p_distance(y, x, p=2)
        kappa = np.exp(-dist / bandwidth)
    elif k_type == "gate":
        dist = p_distance(y, x, p=1)
        kappa = np.ones_like(dist) / bandwidth
        kappa[dist > bandwidth] = 0
    elif k_type == "triangle":
        dist = p_distance(y, x, p=1)
        dist[dist > bandwidth] = bandwidth
        kappa = bandwidth - dist
    else:
        kappa = y @ x.T

    return kappa


def gmm_2d_data(k: int = 3, num: int = 100) -> Tuple[np.ndarray, np.ndarray]:
    centers = 6 * np.random.RandomState(42).randn(k, 2)
    scales = 1 + np.random.RandomState(42).rand(k)
    xs = []
    ys = []
    for i in range(k):
        xs.append(
            scales[i] * np.random.RandomState(i).randn(num, 2)
            + centers[i, :].reshape(1, 2)
        )
        y = np.zeros((num, k))
        y[:, i] = 1
        ys.append(y)
    xs = np.concatenate(xs, axis=0)
    ys = np.concatenate(ys, axis=0)
    return xs, ys


def mean_shift(
    xs: np.ndarray, num_iter: int = 50, k_type: str = "rbf", bandwidth: float = 0.1
) -> np.ndarray:
    """
    Implement the mean-shift algorithm

    :param xs: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param num_iter: the number of iterations
    :param k_type: the type of kernels, including 'rbf', 'gate', 'triangle', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf/gate/triangle kernels
    :return:
        the estimated means with size (N, D)
    """
    # TODO: change the code below and implement the mean-shift algorithm
    for _ in range(num_iter):
        kappa = kernel(xs, k_type=k_type, bandwidth=bandwidth)
        xs = kappa @ xs / np.sum(kappa, axis=1, keepdims=True)
    return xs


def label_propagation(
    xs: np.ndarray,
    ys: np.ndarray,
    num_iter: int = 50,
    k_type: str = "rbf",
    bandwidth: float = 0.1,
) -> np.ndarray:
    """
    Implement the label propagation algorithm

    :param xs: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param ys: a set of labels with size (N, K), where N is the number of samples, K is the number of clusters
        Note that, only few samples are labeled, most of rows are all zeros
    :param num_iter: the number of iterations
    :param k_type: the type of kernels, including 'rbf', 'gate', 'triangle', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf/gate/triangle kernels
    :return:
        the estimated labels after propagation with size (N, D)
    """
    # TODO: change the code below and implement the label propagation algorithm
    # 记录初始有标签的样本（用于每次迭代后固定它们的真实标签不变）
    labeled_mask = np.sum(ys, axis=1) > 0
    ys_init = ys.copy()

    for _ in range(num_iter):
        # 计算核矩阵
        kappa = kernel(xs, k_type=k_type, bandwidth=bandwidth)

        # 计算归一化的分母 (度数矩阵)
        degree = np.sum(kappa, axis=1, keepdims=True)

        # 同时进行标签传播与类似 Mean-Shift 的节点位置更新
        ys = kappa @ ys / degree
        xs = kappa @ xs / degree

        # Clamping，保持已知标注点的真实标签不变
        ys[labeled_mask] = ys_init[labeled_mask]

    return ys


def mean_shift2(
    xs: np.ndarray, num_iter: int = 50, k_type: str = "rbf", bandwidth: float = 0.1
) -> np.ndarray:
    """
    Implement a variant of mean-shift algorithm, with unchanged kernel matrix

    :param xs: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param num_iter: the number of iterations
    :param k_type: the type of kernels, including 'rbf', 'gate', 'triangle', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf/gate/triangle kernels
    :return:
        the estimated means with size (N, D)
    """
    # TODO: change the code below and implement the modified mean-shift
    kappa = kernel(xs, k_type=k_type, bandwidth=bandwidth)
    degree = np.sum(kappa, axis=1, keepdims=True)
    for _ in range(num_iter):
        xs = kappa @ xs / degree
    return xs


def label_propagation2(
    xs: np.ndarray,
    ys: np.ndarray,
    num_iter: int = 50,
    k_type: str = "rbf",
    bandwidth: float = 0.1,
) -> np.ndarray:
    """
    Implement a variant of label propagation algorithm, with a fixed kernel matrix

    :param xs: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param ys: a set of labels with size (N, K), where N is the number of samples, K is the number of clusters
        Note that, only few samples are labeled, most of rows are all zeros
    :param num_iter: the number of iterations
    :param k_type: the type of kernels, including 'rbf', 'gate', 'triangle', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf/gate/triangle kernels
    :return:
        the estimated labels after propagation with size (N, D)
    """
    # TODO: change the code below and implement the modified label-propagation
    # 记录初始有标签的样本（用于每次迭代后固定它们的真实标签不变）
    labeled_mask = np.sum(ys, axis=1) > 0
    ys_init = ys.copy()

    # 核矩阵只计算一次
    kappa = kernel(xs, k_type=k_type, bandwidth=bandwidth)
    degree = np.sum(kappa, axis=1, keepdims=True)

    for _ in range(num_iter):
        # 进行标签传播
        ys = kappa @ ys / degree

        # Clamping，保持已知标注点的真实标签不变
        ys[labeled_mask] = ys_init[labeled_mask]
    return ys


if __name__ == "__main__":
    k = 3
    num = 100
    samples, labels = gmm_2d_data(k=k, num=num)
    ys = np.zeros_like(labels)
    for i in range(k):
        idx = np.random.RandomState(i).permutation(num)
        ys[i * num + idx[:5], i] = 1

    plt.figure()
    for i in range(k):
        visualization_pts(
            pts=samples[i * num : (i + 1) * num, :],
            label="Cluster {}".format(i + 1),
            colors=labels[i * num, :],
        )
    plt.legend()
    plt.savefig("data.png")
    plt.close()

    plt.figure()
    visualization_pts(pts=samples, label="no-label", colors=ys)
    plt.savefig("before_label_propagation.png")
    plt.close()
    for k_type in ["rbf", "gate"]:
        for n in [1, 5,10,15, 20]:
            means1 = mean_shift(xs=samples, num_iter=n, bandwidth=2, k_type=k_type)
            plt.figure()
            for i in range(k):
                visualization_pts(
                    pts=means1[i * num : (i + 1) * num, :],
                    label="Cluster {}".format(i + 1),
                    colors=labels[i * num, :],
                )
            plt.legend()
            plt.savefig("means1_iter{}_{}.png".format(n, k_type))
            plt.close()

            means2 = mean_shift2(xs=samples, num_iter=n, bandwidth=2, k_type=k_type)
            plt.figure()
            for i in range(k):
                visualization_pts(
                    pts=means2[i * num : (i + 1) * num, :],
                    label="Cluster {}".format(i + 1),
                    colors=labels[i * num, :],
                )
            plt.legend()
            plt.savefig("means2_iter{}_{}.png".format(n, k_type))
            plt.close()

            ys1 = label_propagation(
                xs=samples, ys=ys, num_iter=n, bandwidth=2, k_type=k_type
            )
            plt.figure()
            visualization_pts(pts=samples, label="no-label", colors=ys1)
            plt.savefig("after_label_propagation1_iter{}_{}.png".format(n, k_type))
            plt.close()

            ys2 = label_propagation2(
                xs=samples, ys=ys, num_iter=n, bandwidth=2, k_type=k_type
            )
            plt.figure()
            visualization_pts(pts=samples, label="no-label", colors=ys2)
            plt.savefig("after_label_propagation2_iter{}_{}.png".format(n, k_type))
            plt.close()
