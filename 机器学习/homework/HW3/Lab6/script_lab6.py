"""
Introduction to Machine Learning

Lab 6: Nonlinear dimensionality reduction

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab6.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s):
Student ID(s):
"""

import numpy as np
import matplotlib.pyplot as plt
from typing import Tuple
# don't add any other packages


# data simulator and testing function (Don't change them)
def simulate_3d_manifold(n_pts: int = 500, noise_level: float = 0.01, r_seed: int = 42) -> dict:
    """
    Simulate a set of 3D points lying on a manifold, the manifold is a 2D geometry embedded in the 3D space.
    :param n_pts: the number of 3D points
    :param r_seed: the random seed
    :param noise_level: the standard deviation of Gaussian noise
    :return:
        a dictionary containing the 3D points with Gaussian noise and their 2D latent codes.
    """

    t1 = 5 * np.pi / 3 * np.random.RandomState(r_seed).rand(n_pts, 1)
    t2 = 5 * np.pi / 3 * np.random.RandomState(1).rand(n_pts, 1)
    latent_code = np.concatenate((t1, t2), axis=1)
    x1 = 3 + np.cos(t1) * np.cos(t2)
    x2 = 3 + np.cos(t1) * np.sin(t2)
    x3 = np.sin(t1)
    data = np.concatenate((x1, x2, x3), axis=1) + noise_level * np.random.RandomState(r_seed).randn(n_pts, 3)
    return {'3d': data, '2d': latent_code}


def visualization_3d_pts(pts3d: np.ndarray, prefix: str = 'data'):
    fig = plt.figure(figsize=(12, 12))
    ax = fig.add_subplot(projection='3d')
    ax.scatter(pts3d[:, 0], pts3d[:, 1], pts3d[:, 2])
    plt.savefig('{}_3d.png'.format(prefix))
    plt.close()


def visualization_2d_pts(pts2d: np.ndarray, prefix: str = 'data'):
    plt.figure(figsize=(12, 12))
    plt.scatter(pts2d[:, 0], pts2d[:, 1])
    plt.savefig('{}_2d.png'.format(prefix))
    plt.close()


# Task 1: Implement Kernel PCA
def distance_matrix(xs: np.ndarray, distance_type: str = 'L2') -> np.ndarray:
    """
    Construct a N x N distance matrix from a data matrix with size (N, D)
    :param xs: a data matrix with size (N, D)
    :param distance_type: the type of the distance, which can be "L2" or "L1",
        L2 means d_ij = ||xi - xj||_2, while L1 means d_ij = ||xi - xj||_1
    :return:
        a distance matrix with size (N, N)
    """
    # TODO: Change the code below
    # 每个元素代表了两个数据样本的距离
    D = np.zeros((xs.shape[0], xs.shape[0]))
    for i in range(xs.shape[0]):
        for j in range(xs.shape[0]):
            if distance_type == 'L2':
                D[i, j] = np.linalg.norm(xs[i] - xs[j], ord=2)
            elif distance_type == 'L1':
                D[i, j] = np.linalg.norm(xs[i] - xs[j], ord=1)
    return D


def kernel(x: np.ndarray, k_type: str = 'rbf', bandwidth: float = 1) -> np.ndarray:
    """
    Implement typical kernel functions
    1) RBF kernel: k(x, y) = exp(-||x - y||_2^2 / bandwidth)
    2) Linear kernel: k(x, y) = <x, y>

    Hint: Recall your Lab work 4

    :param x: a set of samples with size (N, D), where N is the number of samples, D is the dimension of features
    :param k_type: the type of kernels, including 'rbf', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf kernels
    :return:
        return a matrix with size (M, N)
    """
    # TODO: Change the code below
    if k_type == 'rbf':
        K = np.exp(-distance_matrix(x, distance_type='L2') ** 2 / (2 * bandwidth ** 2))
    elif k_type == 'linear':
        K = np.dot(x, x.T)
    else:
        raise ValueError('Unsupported kernel type: {}'.format(k_type))
    return K


def _center_kernel_matrix(K: np.ndarray) -> np.ndarray:
    # 列中心化kernel matrix
    return K - K.mean(axis=0, keepdims=True)


def kernel_pca(xs: np.ndarray, d: int, k_type: str = 'rbf', bandwidth: float = 1) -> np.ndarray:
    """
    Implement kernel PCA
    :param xs: the data matrix with shape (N, D)
    :param d: the number of dimensions after dimensionality reduction
    :param k_type: the type of kernels, including 'rbf', 'linear'
    :param bandwidth: the hyperparameter controlling the width of rbf kernels
    :return:
    """
    # 建立核并中心化
    K = kernel(xs, k_type=k_type, bandwidth=bandwidth)
    Kc = _center_kernel_matrix(K)

    # 2) SVD分解，排特征值
    eigvals, eigvecs = np.linalg.eigh(Kc)
    order = np.argsort(eigvals)[::-1]
    eigvals = eigvals[order]
    eigvecs = eigvecs[:, order]

    # 3) 取前d个得到降维后的数据
    eigvals_d = np.maximum(eigvals[:d], 0.0)
    eigvecs_d = eigvecs[:, :d]
    return eigvecs_d * np.sqrt(eigvals_d)


# Task 2: Construct a K-NN graph from data points
def construct_knn_graph(xs: np.ndarray, k: int = 5, distance_type: str = 'L2') -> Tuple[np.ndarray, np.ndarray]:
    """
    Construct a K-NN graph from the data points and output the adjacency matrix and the index matrix
    :param xs: a data matrix with (N, D), N is the number of samples, D is the dimension of sample space
    :param k: the number of principal components we would like to output
    :param distance_type: the type of the distance, which can be "L2" or "L1",
        L2 means d_ij = ||xi - xj||_2, while L1 means d_ij = ||xi - xj||_1
    :return:
        an adjacency matrix with size (N, N)
        an index matrix with size (N, k), the n-th row contains the indices of the neighbors of the n-th sample.
    """
    # TODO: change the code below
    N = xs.shape[0]
    if k <= 0 or k >= N:
        raise ValueError("k must satisfy 1 <= k <= N-1")

    D = distance_matrix(xs, distance_type=distance_type)  # (N, N)

    # 排除自身：让对角线变成 +inf，这样最近邻不会选到自己
    D = D.copy()
    np.fill_diagonal(D, np.inf)

    # 取每行最小的 k 个距离对应的索引
    idx = np.argpartition(D, kth=k-1, axis=1)[:, :k]  # (N, k)

    # 构造邻接矩阵
    A = np.zeros((N, N), dtype=float)
    rows = np.arange(N)[:, None]
    A[rows, idx] = 1.0

    # 对称化成无向图
    A = np.maximum(A, A.T)

    return A, idx


# Task 2: Implement the Locally Linear Embedding algorithm
def locally_linear_embedding(xs: np.ndarray, k: int = 5, dim: int = 2, distance_type: str = 'L2') -> np.ndarray:
    """
    Implement the locally linear embedding algorithm
    :param xs: the data matrix with size (N, D), N is the number of samples
    :param k: the number of neighbors per sample in the K-NN graph
    :param dim: the dimension of latent code, where dim < D
    :param distance_type: the type of the distance, which can be "L2" or "L1",
        L2 means d_ij = ||xi - xj||_2, while L1 means d_ij = ||xi - xj||_1
    :return:
        ys: the latent codes of the data, with size (N, dim)
    """
    # TODO: change the code below
    # LLE的重点是继承局部线性关系，首先构建KNN图，计算重构权重，然后通过特征分解得到降维后的数据
    N, _ = xs.shape

    # 1) KNN
    _, idx = construct_knn_graph(xs, k=k, distance_type=distance_type)  # idx: (N, k)

    # 2) 求每个点的重构权重 W
    W = np.zeros((N, N), dtype=float)
    reg = 1e-3  # 正则强度，防止局部协方差矩阵奇异

    ones = np.ones(k, dtype=float)
    for i in range(N):
        nbr = idx[i].astype(int)          # (k,)
        Z = xs[nbr] - xs[i]               # (k, D)
        C = Z @ Z.T                       # (k, k)

        tr = np.trace(C)
        if tr > 0:
            C = C + reg * tr * np.eye(k)
        else:
            C = C + reg * np.eye(k)

        w = np.linalg.solve(C, ones)      # C w = 1
        w = w / w.sum()                   # 约束 sum(w)=1
        W[i, nbr] = w

    # 3) 特征分解：M = (I-W)^T (I-W)，取最小的非零特征向量
    I = np.eye(N)
    M = (I - W).T @ (I - W)
    eigvals, eigvecs = np.linalg.eigh(M)  # 升序

    # 跳过0特征值对应的特征向量
    ys = eigvecs[:, 1:dim + 1]
    return ys


# Task 3: Implement the Laplacian eigenmap algorithm
def laplacian_eigenmaps(xs: np.ndarray, k: int = None, dim: int = 2,
                        normalize: bool = True, bandwidth: float = 4) -> np.ndarray:
    """
    Implement the Laplacian Eigenmap algorithm
    :param xs: the data matrix with size (N, D), N is the number of samples
    :param k: the number of neighbors per sample in the K-NN graph, if k is None, we obtain a fully-connected graph
    :param dim: the dimension of latent code, where dim < D
        L2 means d_ij = ||xi - xj||_2, while L1 means d_ij = ||xi - xj||_1
    :param normalize: use normalized Laplacian or not
    :param bandwidth: the bandwidth of kernel for computing the similarity matrix
    :return:
        ys: the latent codes of the data, with size (N, dim)
    """
    # TODO: change the code below
    N, D = xs.shape

    # 1) 相似度矩阵 W（用 RBF 权重）
    dist = distance_matrix(xs, distance_type='L2')  # (N, N)
    W_full = np.exp(-(dist ** 2) / (2.0 * (bandwidth ** 2)))
    np.fill_diagonal(W_full, 0.0)

    if k is None:
        W = W_full
    else:
        A, _ = construct_knn_graph(xs, k=k, distance_type='L2')  # A: (N,N) 0/1 且已对称化
        W = W_full * A
        np.fill_diagonal(W, 0.0)
        W = np.maximum(W, W.T)

    # 2) 拉普拉斯矩阵
    deg = W.sum(axis=1)  # (N,)

    if normalize:
        # L_sym = I - D^{-1/2} W D^{-1/2}
        deg_inv_sqrt = np.zeros_like(deg)
        mask = deg > 1e-12
        deg_inv_sqrt[mask] = 1.0 / np.sqrt(deg[mask])
        S = (deg_inv_sqrt[:, None] * W) * deg_inv_sqrt[None, :]
        L = np.eye(N) - S
        eigvals, eigvecs = np.linalg.eigh(L)  # 升序
    else:
        # L = D - W
        L = np.diag(deg) - W
        eigvals, eigvecs = np.linalg.eigh(L)  # 升序

    # 3) 取最小的非零特征向量
    ys = eigvecs[:, 1:dim + 1]
    return ys

# Testing script
if __name__ == '__main__':
    data = simulate_3d_manifold()
    visualization_3d_pts(data['3d'], prefix='data')
    visualization_2d_pts(data['2d'], prefix='data')
    for h in [0.01, 0.1, 1, 10, 100]:
        z0 = kernel_pca(xs=data['3d'], d=2, k_type='rbf', bandwidth=h)
        visualization_2d_pts(z0, prefix='KPCA_rbf_{}'.format(int(np.log10(h))))
    z1 = kernel_pca(xs=data['3d'], d=2, k_type='linear')
    visualization_2d_pts(z1, prefix='KPCA_linear')

    for k in [3, 5, 10, 25, 50, 100, 200]:
        z1 = locally_linear_embedding(xs=data['3d'], k=k)
        visualization_2d_pts(z1, prefix='LLE_{}'.format(k))

    for k in [3, 5, 10, 25, 50, 100, 200, None]:
        for normalize in [True, False]:
            z2 = laplacian_eigenmaps(xs=data['3d'], k=k, normalize=normalize)
            if k is None:
                prefix = 'LE_full_{}'.format(normalize)
            else:
                prefix = 'LE_{}_{}'.format(k, normalize)
            visualization_2d_pts(z2, prefix=prefix)
