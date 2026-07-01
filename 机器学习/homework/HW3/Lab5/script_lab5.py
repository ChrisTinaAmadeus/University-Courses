"""
Introduction to Machine Learning

Lab 5: Compressive Sensing

TODO: Add your information here.
    IMPORTANT: Please ensure this script
    (1) Run script_lab4.py on Python >=3.6;
    (2) No errors;
    (3) Finish in tolerable time on a single CPU (e.g., <=10 mins);
Student name(s):
Student ID(s):
"""

import copy
import numpy as np
import matplotlib.pyplot as plt
from typing import Tuple

import scipy.linalg


# don't add any other packages


# Task 1: Implement Sparse Data Generation Function
def sparse_data(dictionary: np.ndarray, sparsity: int = 2, n: int = 1, random_seed: int = 42) -> np.ndarray:
    """
    Implement PCA via eigen-decomposition
    :param dictionary: a dictionary matrix with (D, K), D is the dimension of data, K is the number of atoms/columns in
    the dictionary
    :param sparsity: the number of nonzero coefficients used to construct the data
    :param n: the number of samples in the data
    :param random_seed: the random seed used to generate coefficients.
    :return:
        the zero-mean data with size (N, D)
    """
    D, K = dictionary.shape
    rng = np.random.RandomState(random_seed)
    xs = np.zeros((n, D))

    for i in range(n):
        coeffs = np.zeros(K)
        indices = rng.choice(K, sparsity, replace=False)
        coeffs[indices] = rng.randn(sparsity)
        x = dictionary @ coeffs
        xs[i] = x - np.mean(x)

    return xs


# Task 2: Implement the random projection
def random_projection(xs: np.ndarray, dim: int = 10, sense_type: str = 'normal', random_seed: int = 10) -> \
        Tuple[np.ndarray, np.ndarray]:
    """
    Generate random projection matrix and project data to low-dimensional space
    :param xs: the data matrix with size (N, D), N is the number of samples
    :param dim: the dimension of output
    :param sense_type: 'normal' or 'bernoulli', determining the type of random projection matrix
    :param random_seed: the random seed used to generate the random projection matrix
    :return:
        ys: the projected data with size (N, dim)
        proj: the random projection matrix with size (D, dim)
    """
    D = xs.shape[1]
    rng = np.random.RandomState(random_seed)

    if sense_type == 'normal':
        proj = rng.randn(D, dim)
    elif sense_type == 'bernoulli':
        proj = rng.choice([-1, 1], size=(D, dim)).astype(float)
    else:
        raise ValueError("sense_type must be 'normal' or 'bernoulli'")

    ys = xs @ proj
    return ys, proj


def _soft_thresh(x: np.ndarray, thresh: float) -> np.ndarray:
    """Soft thresholding operator for L1 regularization."""
    return np.sign(x) * np.maximum(np.abs(x) - thresh, 0)


# Task 3: Implement the data recovery algorithm
def data_recovery(ys: np.ndarray, dictionary: np.ndarray, proj: np.ndarray) -> np.ndarray:
    """
    Recover sparse data from compressed measurements via Lasso
    (Coordinate Descent with soft thresholding).
    :param ys: the random projection result with size (N, dim)
    :param dictionary: a dictionary matrix with (D, K)
    :param proj: the random projection matrix with size (D, dim)
    :return:
        xs: the recovery data matrix with size (N, D)
    """
    N = ys.shape[0]
    D, K = dictionary.shape

    # Sensing matrix: Theta = proj^T @ dictionary,  shape (dim, K)
    Theta = proj.T @ dictionary

    # Normalize columns of Theta for numerical stability
    col_norms = np.linalg.norm(Theta, axis=0)
    col_norms[col_norms == 0] = 1
    Theta = Theta / col_norms

    lambda_param = 0.1
    max_iter = 2000
    tol = 1e-8

    # Precompute Theta^T for the inner product in each coordinate update
    Theta_T = Theta.T

    xs = np.zeros((N, D))

    for i in range(N):
        y = ys[i]
        c = np.zeros(K)
        r = y.copy()  # residual = y - Theta @ c (initially c=0, so r=y)

        for _ in range(max_iter):
            c_old = c.copy()

            # Coordinate descent: sweep through all coordinates
            for j in range(K):
                # Inner product of Theta[:, j] with residual, plus current c[j]
                # (since ||Theta[:, j]||^2 = 1 after normalization)
                rho_j = Theta_T[j] @ r + c[j]
                c_new = _soft_thresh(rho_j, lambda_param)
                # Update residual: r = r + Theta[:, j] * (c[j] - c_new)
                r += Theta[:, j] * (c[j] - c_new)
                c[j] = c_new

            if np.linalg.norm(c - c_old) < tol:
                break

        xs[i] = dictionary @ (c / col_norms)

    return xs


# Task 4: Visualize the covariance matrix
def visualization_cov(xs: np.ndarray):
    """
    Visualize the covariance matrix of data
    :param xs: a data matrix with size (N, D)
    :return: (visualize)
        cov: the covariance matrix with size (D, D)
    """
    # Need at least 2 samples to compute covariance
    if xs.shape[0] <= 1:
        cov = np.zeros((xs.shape[1], xs.shape[1]))
    else:
        cov = np.cov(xs, rowvar=False)
    plt.imshow(cov)
    plt.colorbar()


# Testing script
if __name__ == '__main__':
    dictionary = scipy.linalg.hadamard(128, dtype=float)
    print(dictionary)
    data = sparse_data(dictionary,sparsity=1,n=100,random_seed=42)
    plt.figure()
    visualization_cov(data)
    plt.title('real data cov')
    plt.savefig('data_cov.png')
    plt.close('all')

    for sense_type in ['normal', 'bernoulli']:
        for dim in [4, 8, 16, 32]:
            ys, proj = random_projection(xs=data, dim=dim, sense_type=sense_type)
            xs = data_recovery(ys=ys, dictionary=dictionary, proj=proj)
            print('SenseType={}, Dim={}, MSE={}'.format(sense_type, dim, np.sum((data-xs)**2)))
            plt.figure()
            visualization_cov(xs)
            plt.title('est data cov')
            plt.savefig('est_data_cov_{}_{}.png'.format(sense_type, dim))
            plt.close('all')

            plt.figure()
            visualization_cov(ys)
            plt.title('cs cov')
            plt.savefig('cs_data_cov_{}_{}.png'.format(sense_type, dim))
            plt.close('all')
