"""
Introduction to Machine Learning

Lab 7: Gaussian mixture model: its application to point cloud alignment

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab7.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s):王松宸
Student ID(s):2024201594
"""

from scipy.io import loadmat
import matplotlib.pyplot as plt
import numpy as np
from typing import Tuple

# don't add any other packages


def squared_distance_matrix(xs: np.ndarray, ys: np.ndarray) -> np.ndarray:
    """
    Construct a N x M distance matrix from a data matrix with size (N, D)
    Each element d_{ij} = ||x_i - y_j ||_2^2

    :param xs: a set of points with size (N, D), N is the number of samples, D is the dimension of points
    :param ys: a set of points with size (M, D), M is the number of samples, D is the dimension of points
    :return:
        a distance matrix with size (N, M)
    """
    # TODO: change the code below and implement your distance computation method.
    Distance = (
        np.sum(xs**2, axis=1, keepdims=True) + np.sum(ys**2, axis=1) - 2 * xs @ ys.T
    )
    return Distance


def estimate_variance(
    xs: np.ndarray,
    ys: np.ndarray,
    affine: np.ndarray,
    translation: np.ndarray,
    responsibility: np.ndarray,
) -> float:
    """
    Estimate the variance of GMM.
    For simplification, we assume all the Gaussian distributions share the same variance,
    and each feature dimension is independent, so the variance can be represented as a scalar.

    :param xs: a set of points with size (N, D), N is the number of samples, D is the dimension of points
    :param ys: a set of points with size (M, D), M is the number of samples, D is the dimension of points
    :param affine: an affine matrix with size (D, D)
    :param translation: a translation vector with size (1, D)
    :param responsibility: the responsibility matrix with size (N, M)
    :return:
        the variance of each Gaussian distribution, a float
    """
    # TODO: change the code below and compute the variance of each Gaussian
    # 计算得到每个点在 k 步的 A 和 k 步的 t 后的变换结果
    y_transformed = ys @ affine + translation
    # 距离矩阵：用于计算每个点与变换后点的距离
    distance_matrix = squared_distance_matrix(xs, y_transformed)
    # 利用责任度矩阵和距离矩阵计算方差
    # 这个方差实际上是用期望来算的，概率乘以距离的平方和，除以总的责任度和点的数量
    variance = np.sum(responsibility * distance_matrix) / (
        np.sum(responsibility) * xs.shape[1]
    )
    return variance


def e_step(
    xs: np.ndarray,
    ys: np.ndarray,
    affine: np.ndarray,
    translation: np.ndarray,
    variance: float,
) -> np.ndarray:
    """
    The e-step of the em algorithm, estimating the responsibility P=[p(y_m | x_n)] based on current model

    :param xs: a set of points with size (N, D), N is the number of samples, D is the dimension of points
    :param ys: a set of points with size (M, D), M is the number of samples, D is the dimension of points
    :param affine: an affine matrix with size (D, D)
    :param translation: a translation vector with size (1, D)
    :param variance: a float controlling the variance of each Gaussian component
    :return:
        the responsibility matrix P=[p(y_m | x_n)] with size (N, M),
        which row is the conditional probability of clusters given the n-th sample x_n
    """
    # TODO: Change the code below and implement the E-step of GMM
    # 计算 ys 的点是由已知的 xs 的点变换来的概率（后验概率）
    distance_matrix = squared_distance_matrix(xs, ys @ affine + translation)
    responsibility = np.exp(-0.5 / variance * distance_matrix)
    responsibility = responsibility / np.sum(responsibility, axis=1, keepdims=True)
    return responsibility


def m_step(
    xs: np.ndarray, ys: np.ndarray, responsibility: np.ndarray
) -> Tuple[np.ndarray, np.ndarray, float, np.ndarray]:
    """
    the m-step of the em algorithm:

    min_{affine, translation, variance} 1/(2*variance) * sum_{m,n} p(y_m | x_n) ||x_n - affine y_m - translation||_2^2

    :param xs: a set of points with size (N, D), N is the number of samples, D is the dimension of points
    :param ys: a set of points with size (M, D), M is the number of samples, D is the dimension of points
    :param responsibility: the responsibility matrix P=[p(y_m | x_n)] with size (N, M)
    :return:
        an affine matrix with size (D, D)
        a translation vector with size (D, 1)
        the variance of GMM, a float
        the registered point cloud ys_new, with size (M, D)
    """
    # 分别优化 A 和 t 的更新公式，A 的更新公式是基于最小二乘法的，t 的更新公式是基于均值的
    dim = xs.shape[1]
    weight_x = np.sum(responsibility, axis=1)
    weight_y = np.sum(responsibility, axis=0)
    total_weight = np.sum(weight_y)

    mean_x = (weight_x @ xs) / total_weight
    mean_y = (weight_y @ ys) / total_weight
    xs_centered = xs - mean_x
    ys_centered = ys - mean_y

    lhs = ys_centered.T @ (weight_y[:, np.newaxis] * ys_centered)
    rhs = ys_centered.T @ responsibility.T @ xs_centered
    try:
        affine = np.linalg.solve(lhs, rhs)
    except np.linalg.LinAlgError:
        affine = np.linalg.pinv(lhs) @ rhs

    translation = mean_x - mean_y @ affine
    ys_new = ys @ affine + translation
    distance_matrix = squared_distance_matrix(xs, ys_new)
    variance = np.sum(responsibility * distance_matrix) / (total_weight * dim)

    return affine, translation.reshape(1, dim), variance, ys_new


def em_for_alignment(
    xs: np.ndarray, ys: np.ndarray, num_iter: int = 100
) -> Tuple[np.ndarray, np.ndarray]:
    """
    The em algorithm for aligning two point clouds based on affine transformation
    :param xs: a set of points with size (N, D), N is the number of samples, D is the dimension of points
    :param ys: a set of points with size (M, D), M is the number of samples, D is the dimension of points
    :param num_iter: the number of EM iterations
    :return:
        ys_new: the aligned points: ys_new = ys @ affine + translation
        responsibility: the responsibility matrix P=[p(y_m | x_n)] with size (N, M),
        whose elements indicating the correspondence between the points
    """
    ys_new = np.zeros_like(ys)
    # initialize model parameters:
    responsibility = np.ones((xs.shape[0], ys.shape[0])) / ys.shape[0]
    dim = xs.shape[1]
    affine = np.eye(dim)
    translation = np.zeros((1, dim))
    variance = estimate_variance(xs, ys, affine, translation, responsibility)
    # TODO: implement the EM algorithm of GMM below for point cloud alignment
    for _ in range(num_iter):
        # E-step: estimate the responsibility matrix based on current model parameters
        responsibility = e_step(xs, ys, affine, translation, variance)
        # M-step: update the model parameters based on current responsibility matrix
        affine, translation, variance, ys_new = m_step(xs, ys, responsibility)
    return ys_new, responsibility


if __name__ == "__main__":
    fish = loadmat("fish.mat")
    xs = fish["X"]
    ys = fish["Y"]
    ys_new, prob = em_for_alignment(xs, ys)

    plt.figure()
    plt.scatter(xs[:, 0], xs[:, 1], label="target")
    plt.scatter(ys[:, 0], ys[:, 1], label="source")
    plt.scatter(ys_new[:, 0], ys_new[:, 1], label="aligned source")
    plt.legend()
    plt.savefig("result.png")
    plt.close()

    plt.figure()
    plt.scatter(xs[:, 0], xs[:, 1])
    plt.scatter(ys[:, 0], ys[:, 1])
    idx = np.argmax(prob, axis=1)
    for n in range(xs.shape[0]):
        plt.plot([xs[n, 0], ys[idx[n], 0]], [xs[n, 1], ys[idx[n], 1]], "k-")
    plt.savefig("correspondence.png")
    plt.close()
