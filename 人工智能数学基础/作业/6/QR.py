#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
使用 QR 迭代法求 3x3 矩阵的特征值，并在每一步打印 Q 与 R。

说明：
- 迭代公式：对 A_k 做 QR 分解 A_k = Q_k R_k，然后令 A_{k+1} = R_k Q_k。
- 当严格下三角部分的范数足够小（或达到最大迭代次数）时停止。
- 最终从对角线提取近似特征值。

注意：若矩阵存在复特征值，纯实数的无位移 QR 迭代将收敛到实 Schur 形，可能出现 2x2 实块，
此时直接取对角线得到的并非全部真实特征值。为简洁起见，这里假设输入矩阵拥有实特征值或
在给定容差下对角占优可直接读取。
"""

from __future__ import annotations

import sys
from typing import Tuple

import numpy as np


def strictly_lower_fro_norm(A: np.ndarray) -> float:
    """返回矩阵严格下三角部分（不含对角）的 Frobenius 范数。"""
    i_lower = np.tril_indices(A.shape[0], k=-1)
    return np.linalg.norm(A[i_lower])


def qr_iteration(
    A: np.ndarray,
    max_iter: int = 50,
    tol: float = 1e-10,
    verbose: bool = True,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    对给定矩阵执行 QR 迭代。

    参数:
            A: 形状为 (3, 3) 的实矩阵（np.ndarray）。
            max_iter: 最大迭代次数。
            tol: 收敛阈值（严格下三角部分 Frobenius 范数）。
            verbose: 若为 True，则在每次迭代打印 Q 与 R。

    返回:
            (A_k, eigvals_diag)
            A_k: 迭代结束时的矩阵（应接近上三角）。
            eigvals_diag: 从 A_k 的对角线读取的特征值近似（shape=(3,)）。
    """
    A_k = np.array(A, dtype=float, copy=True)

    # 打印格式设置（不影响数值，只影响显示）
    print_options = dict(precision=6, suppress=True)

    for k in range(1, max_iter + 1):
        Q, R = np.linalg.qr(A_k)

        if verbose:
            print(f"\nIteration {k}:")
            with np.printoptions(**print_options):
                print("Q =")
                print(Q)
                print("R =")
                print(R)

        A_k = R @ Q

        # 收敛判据：严格下三角部分的范数小于阈值
        off_norm = strictly_lower_fro_norm(A_k)
        if off_norm < tol:
            break

    eigvals_diag = np.diag(A_k).copy()
    return A_k, eigvals_diag


def read_matrix_from_input() -> np.ndarray:
    """
    交互读取用户输入的 3x3 矩阵。
    支持逐行输入（每行 3 个数，以空格分隔）。
    """
    rows = []
    for i in range(3):
        while True:
            try:
                line = input(f"请输入第 {i+1} 行的3个数（以空格分隔）：").strip()
                nums = [float(x) for x in line.split()]
                if len(nums) != 3:
                    raise ValueError("需要恰好3个数。")
                rows.append(nums)
                break
            except ValueError as e:
                print(f"输入无效：{e}，请重试。")
    A = np.array(rows, dtype=float)
    return A


def main():
    print("QR 迭代法求 3x3 矩阵特征值")
    print("提示：依次输入 3 行，每行 3 个浮点数，以空格分隔。")

    A = read_matrix_from_input()

    # 可根据需要调整这两个参数
    max_iter = 50
    tol = 1e-10

    print("\n初始矩阵 A =")
    with np.printoptions(precision=6, suppress=True):
        print(A)

    A_k, eigs = qr_iteration(A, max_iter=max_iter, tol=tol, verbose=True)

    print("\n迭代结束的矩阵 A_k （应接近上三角）=")
    with np.printoptions(precision=6, suppress=True):
        print(A_k)

    print("\n从对角线提取的特征值（近似）=")
    with np.printoptions(precision=12, suppress=True):
        print(eigs)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n已中断。")
        sys.exit(1)
