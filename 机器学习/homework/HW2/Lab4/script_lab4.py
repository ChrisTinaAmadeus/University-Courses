"""
Introduction to Machine Learning

Lab 5: Matrix factorization and linear dimensionality reduction

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab4.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s): 王松宸
Student ID(s): 2024201594
"""

import copy
import numpy as np
import matplotlib.pyplot as plt
from typing import Tuple
# don't add any other packages


# data simulator and testing function (Don't change them)
def zero_mean_point_cloud_simulator(n_pts: int = 50,
                                    r_seed: int = 42) -> dict:
    """
    Simulate a set of zero-mean 2D points with Gaussian noise or outliers
    :param n_pts: the number of 2D points
    :param r_seed: the random seed
    :return:
        a dictionary containing the points with Gaussian noise and those with outliers, respectively
    """
    x = 4 * (np.random.RandomState(r_seed).rand(n_pts, 1) - 0.5)
    y = 0.4 * x
    data = np.concatenate((x, y), axis=1)
    pts1 = data + 0.1 * np.random.RandomState(r_seed).randn(n_pts, 2)
    pts2 = data + 0.01 * np.random.RandomState(r_seed).randn(n_pts, 2)
    idx = np.random.RandomState(r_seed).permutation(n_pts)
    n_noise = int(0.2 * n_pts)
    pts2[idx[:n_noise], :] = np.random.RandomState(r_seed).randn(n_noise, 2) + np.array([0.5, 1.5]).reshape((1, 2))
    return {'gauss': pts1, 'outlier': pts2}


def visualization_pts(pts: np.ndarray, label: str, point_type: str):
    plt.plot(pts[:, 0], pts[:, 1], point_type, label=label)


def visualization_line(v: np.ndarray, label: str, line_type: str):
    xs = 5 * (np.arange(0, 100) / 100 - 0.5)
    ys = v[1] / v[0] * xs
    plt.plot(xs, ys, line_type, label=label)


# Task 1: Implement PCA via eigen-decomposition
def pca(xs: np.ndarray, n_pc: int = 2) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """
    Implement PCA via eigen-decomposition
    :param xs: a data matrix with (N, D), N is the number of samples, D is the dimension of sample space
    :param n_pc: the number of principal components we would like to output
    :return:
        the matrix containing top-k principal components, with size (D, n_pc)
        the vector indicating the top-k eigenvalues, with size (n_pc)
        the data recovered from the projections along the principal components, with size (N, D)
        the zero-mean data with size (N, D)
    """
    # TODO: Change the code below to implement your PCA algorithm
    # 选择使用特征值分解
    n_samp, dim = xs.shape

    mean_x = np.mean(xs, axis=0, keepdims=True)
    xs0 = xs - mean_x

    denom = float(max(1, n_samp))
    cov = (xs0.T @ xs0) / denom

    eigvals, eigvecs = np.linalg.eigh(cov)
    order = np.argsort(eigvals)[::-1]
    eigvals = eigvals[order]
    eigvecs = eigvecs[:, order]

    k = int(min(max(1, n_pc), dim))
    vs = eigvecs[:, :k]
    lambdas = eigvals[:k]

    zs = xs0 @ vs
    xhat0 = zs @ vs.T
    return vs, lambdas, xhat0, xs0


# Task 2: Implement data whitening via the method in Lecture 2 and the PCA-based method in Lecture 5
def data_whitening(xs: np.ndarray) -> np.ndarray:
    """
    Implement data whitening via the method in Lecture 2 or PCA
    :param xs: the data matrix with size (N, D), N is the number of samples
    :return:
        ys: the data yield normal distribution, with size (N, D)
    """
    # TODO: Change the code below and implement your data whitening method (Hint: you can call the above PCA function)

    dim = xs.shape[1]

    mean_x = np.mean(xs, axis=0, keepdims=True)
    xs0 = xs - mean_x

    vs, lambdas, _, _ = pca(xs, n_pc=dim)

    eps = 1e-12
    lambdas = np.maximum(lambdas, 0.0)

    zs = xs0 @ vs
    inv_sqrt = 1.0 / np.sqrt(lambdas + eps)
    ys_pca = zs * inv_sqrt.reshape((1, -1))

    ys = ys_pca @ vs.T
    return ys


# Task 3: Try to develop your own method to achieve robust PCA (the method may not be the state-of-the-art, but doable)
def hard_thresholding(x: np.ndarray, ratio: float) -> np.ndarray:
    """
    The hard-thresholding operator
    :param x: input array with arbitrary size
    :param ratio: the ratio of nonzero elements
    :return:
        y = x,  if |x| > a threshold
            0,  otherwise
    """
    if ratio <= 0:
        return np.zeros_like(x)
    if ratio >= 1:
        return copy.deepcopy(x)

    x_flat = x.reshape((-1,))
    num = x_flat.size
    k = int(np.floor(ratio * num))
    if k < 1:
        return np.zeros_like(x)

    abs_flat = np.abs(x_flat)

    # 找到 k 个绝对值最大的元素的索引
    idx_topk = np.argpartition(abs_flat, -k)[-k:]
    mask = np.zeros((num,), dtype=bool)
    mask[idx_topk] = True

    y_flat = np.zeros_like(x_flat)
    y_flat[mask] = x_flat[mask]
    return y_flat.reshape(x.shape)


def robust_pca_hard(xs: np.ndarray, n_pc: int = 2, n_alt: int = 100,
                    ratio_nz: float = 0.1) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """
    Implement your own algorithm to solve the robust PCA problem via
    optimizing the low-rank factorization of data matrix (X in R^{N x D}) explicitly, i.e.,

    min_{L, S} ||X - (L + S)||_F^2
    s.t. rank(L) <= n_pc, ||S||_0 < ratio_nz * (N * D)

    Hint: you may want to solve L and S in an alternating optimization manner:
    1) Fix L and solve
        L = argmin_L ||X - (L + S)||_F^2
        s.t. rank(L) <= n_pc
    2) Fix S and solve
        S = argmin_S ||X - (L + S)||_F^2,
        s.t.. ||S||_0 < ratio_nz * (N * D)

    :param xs: a data matrix with (N, D), N is the number of samples, D is the dimension of sample space.
    :param n_pc: the number of principal components we would like to output.
    :param n_alt: the number of steps for alternating optimization.
    :param ratio_nz: the ratio of non-zero elements in the whole matrix.
    :return:
        the matrix containing top-k principal components, with size (D, n_pc)
        the vector indicating the top-k eigenvalues, with size (n_pc)
        the data recovered from the projections along the principal components, with size (N, D)
        the zero-mean data with size (N, D)
    """

    n_samp,dim = xs.shape

    k = int(min(max(1, n_pc), dim))

    mean_x = np.mean(xs, axis=0, keepdims=True)
    xs0 = xs - mean_x

    sparse = np.zeros_like(xs0)

    def best_rank_k_approx(mat: np.ndarray, rank: int) -> np.ndarray:
        """Compute argmin_{rank(L)<=rank} ||mat - L||_F^2 via eigen-decomposition of mat^T mat."""
        gram = mat.T @ mat  # (D, D)
        evals, evecs = np.linalg.eigh(gram)
        order = np.argsort(evals)[::-1]
        evecs = evecs[:, order]
        vk = evecs[:, :rank]
        return mat @ vk @ vk.T

    low_rank = best_rank_k_approx(xs0, k)

    for _ in range(int(max(1, n_alt))):
        # 用 rank-k approximation 更新 L
        low_rank = best_rank_k_approx(xs0 - sparse, k)
        # 用 hard-thresholding 更新 S
        resid = xs0 - low_rank
        sparse = hard_thresholding(resid, ratio=ratio_nz)

    denom = float(max(1, n_samp))
    cov_l = (low_rank.T @ low_rank) / denom
    eigvals, eigvecs = np.linalg.eigh(cov_l)
    order = np.argsort(eigvals)[::-1]
    eigvals = eigvals[order]
    eigvecs = eigvecs[:, order]

    vs = eigvecs[:, :k]
    lambdas = eigvals[:k]
    xhat0 = low_rank
    return vs, lambdas, xhat0, xs0


# Task 4: Suppose that you are a data attacker. Because of limited budgets, you can only add two outliers
# Try to design a "data poisoning" strategy to change the covariance of the data as much as possible.
def coupled_outlier_poisoning(xs: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    """
    Generate two outliers "x1" and "x2", with constraints ||x1||_2 = ||x2||_2 = 1 and x1 + x2 = 0
    :param xs: a data matrix with size (N, D), N is the number of samples
    :return:
        the outliers with size (2, D)
        the new data matrix with the outlier, with size (N+2, D)
    """
    n_samp, dim = xs.shape
    if n_samp < 1 or dim < 1:
        raise ValueError('xs must have positive shape (N, D).')

    # 中心化数据并计算协方差矩阵
    mean_x = np.mean(xs, axis=0, keepdims=True)
    xs0 = xs - mean_x
    denom = float(max(1, n_samp))
    cov = (xs0.T @ xs0) / denom

    eigvals, eigvecs = np.linalg.eigh(cov)
    idx_min = int(np.argmin(eigvals))
    v_min = eigvecs[:, idx_min]
    v_min = v_min / (np.linalg.norm(v_min) + 1e-12)

    x1 = v_min.reshape((1, dim))
    x2 = (-v_min).reshape((1, dim))
    outliers = np.concatenate([x1, x2], axis=0)

    data_noisy = np.concatenate([xs, outliers], axis=0)
    return outliers, data_noisy


# Task 5: implement the NMF algorithm
def nonnegative_matrix_factorization(xs: np.ndarray,
                                     rank: int,
                                     num_iter: int = 100,
                                     seed: int = 1) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Implement the nonnegative matrix factorization

    min_{U, V} ||X - UV^T||_F^2

    s.t. U in [0, inf]^{(N, r)} and V in [0, inf]^{(D, r)}

    :param xs: a data matrix with size (N, D), N is the number of samples
    :param rank: the rank of U and V
    :param num_iter: the number of iterations
    :param seed: the random seed of initialization
    :return:
        U in [0, inf]^{(N, r)}
        V in [0, inf]^{(D, r)}
        hat{X} = UV^T
    """
    us = np.random.RandomState(seed=seed).rand(xs.shape[0], rank)
    vs = np.random.RandomState(seed=seed + 2).rand(xs.shape[1], rank)
    eps = 1e-12

    # U <- U * (X V) / (U (V^T V))
    # V <- V * (X^T U) / (V (U^T U))
    for _ in range(int(max(1, num_iter))):
        xvt = xs @ vs
        vtv = vs.T @ vs
        denom_u = us @ vtv
        us = us * (xvt / (denom_u + eps))

        xtu = xs.T @ us
        utu = us.T @ us
        denom_v = vs @ utu
        vs = vs * (xtu / (denom_v + eps))

        # 确保 U 和 V 中的元素非负
        us = np.maximum(us, 0.0)
        vs = np.maximum(vs, 0.0)

    return us, vs, us @ vs.T


# Testing script
if __name__ == '__main__':
    data = zero_mean_point_cloud_simulator()
    for noise_type in data.keys():
        vs1, lambdas1, xhat1, xs1 = pca(data[noise_type], n_pc=1)
        vs2, lambdas2, xhat2, _ = robust_pca_hard(data[noise_type], n_pc=1, ratio_nz=0.1)
        xhat3 = data_whitening(data[noise_type])

        plt.figure()
        visualization_pts(xs1, label='data points', point_type='g.')
        visualization_pts(xhat1, label='pca', point_type='rx')
        visualization_pts(xhat2, label='rpca', point_type='bx')
        visualization_line(v=vs1, label='pca v1', line_type='r:')
        visualization_line(v=vs2, label='rpca v1', line_type='b:')
        visualization_line(v=np.array([1, 0.4]), label='real pc', line_type='g:')
        result = 'PCA vs RPCA: {} noise'.format(noise_type)
        plt.title(result)
        plt.legend()
        plt.savefig('result_{}.png'.format(noise_type))
        plt.close('all')

        plt.figure()
        visualization_pts(data[noise_type], label='before whitening', point_type='g.')
        visualization_pts(xhat3, label='after whitening', point_type='rx')
        plt.legend()
        plt.axis('equal')
        plt.savefig('whitening_{}.png'.format(noise_type))
        plt.close('all')

    vs1, lambdas1, xhat1, xs1 = pca(data['gauss'], n_pc=1)
    outliers, data_noisy = coupled_outlier_poisoning(data['gauss'])
    print(data['gauss'].shape, data_noisy.shape)
    vs2, lambdas2, xhat2, _ = pca(data_noisy, n_pc=1)
    plt.figure()
    visualization_pts(data['gauss'], label='data points', point_type='g.')
    visualization_pts(outliers, label='outlier', point_type='k*')
    visualization_pts(xhat1, label='PCA before poisoning', point_type='rx')
    visualization_pts(xhat2, label='PCA after poisoning', point_type='bx')
    visualization_line(v=vs1, label='v1 before poisoning', line_type='r:')
    visualization_line(v=vs2, label='v1 after poisoning', line_type='b:')
    visualization_line(v=np.array([1, 0.4]), label='real pc', line_type='g:')
    result = 'Covariance poisoning'
    plt.title(result)
    plt.legend()
    plt.axis('equal')
    plt.savefig('poisoning_pca.png')
    plt.close('all')

    data_mat = np.random.RandomState(seed=42).rand(100, 50)
    for r in [5, 10, 20, 30, 40]:
        u_mat, v_mat, data_approx = nonnegative_matrix_factorization(xs=data_mat, rank=r, num_iter=100, seed=1)
        error = np.sum(np.abs(data_mat - data_approx)) / np.sum(data_mat)
        print('Rank-{} NMF approximation RMAE={}'.format(r, error))
