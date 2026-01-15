from __future__ import annotations

import argparse
from pathlib import Path
from typing import Optional, Tuple

import numpy as np
import matplotlib.pyplot as plt
from sklearn.decomposition import PCA

plt.rcParams["font.sans-serif"] = ["SimHei"]
plt.rcParams["axes.unicode_minus"] = False


def load_embeddings(
    npz_path: str | Path,
) -> Tuple[np.ndarray, Optional[np.ndarray], Optional[np.ndarray]]:
    """
    读取由 wenlan_feature_extractor 生成的 .npz 文件。

    返回:
      X: (N, D) float32 特征
      y: (N,) int64 标签（可能为 None）
      paths: (N,) object 图片路径（可能为 None）
    """
    npz_path = Path(npz_path)
    if not npz_path.exists():
        raise FileNotFoundError(f"找不到特征文件: {npz_path}")
    data = np.load(npz_path, allow_pickle=True)

    X = data["features"].astype(np.float32)
    y = data["labels"] if "labels" in data.files else None
    paths = data["paths"] if "paths" in data.files else None
    return X, y, paths


def l2_normalize(X: np.ndarray, eps: float = 1e-12) -> np.ndarray:
    norm = np.linalg.norm(X, axis=1, keepdims=True)
    norm = np.maximum(norm, eps)
    return X / norm


def standardize(X: np.ndarray, eps: float = 1e-6) -> np.ndarray:
    mean = X.mean(axis=0, keepdims=True)
    std = X.std(axis=0, keepdims=True)
    std = np.where(std < eps, 1.0, std)
    return (X - mean) / std


class KMeans:
    """K-means 简洁实现（按注释步骤）。"""

    def __init__(
        self,
        n_clusters: int = 2,
        max_iter: int = 100,
        tol: float = 1e-9,
        random_state: Optional[int] = 42,
    ):
        self.n_clusters = n_clusters
        self.max_iter = max_iter
        self.tol = tol
        self.random_state = random_state
        self.cluster_centers_: Optional[np.ndarray] = None
        self.labels_: Optional[np.ndarray] = None
        self.n_iter_: int = 0

    def _init_centers(self, X: np.ndarray) -> np.ndarray:
        rng = np.random.default_rng(self.random_state)
        n_samples = X.shape[0]
        # 随机选择 k 个不同样本作为初始中心
        indices = rng.choice(n_samples, size=self.n_clusters, replace=False)
        return X[indices].copy()

    def _assign(self, X: np.ndarray, centers: np.ndarray) -> np.ndarray:
        # 计算每个样本到每个中心的平方距离，并取最近的中心
        # distances 的形状为 (N, K)
        distances = []
        for j in range(self.n_clusters):
            diff = X - centers[j]
            distances.append(np.sum(diff * diff, axis=1))
        distances = np.stack(distances, axis=1)
        return np.argmin(distances, axis=1)

    def _update_centers(
        self, X: np.ndarray, labels: np.ndarray, old_centers: np.ndarray
    ) -> np.ndarray:
        rng = np.random.default_rng(self.random_state)
        new_centers = old_centers.copy()
        for j in range(self.n_clusters):
            mask = labels == j
            if np.any(mask):
                new_centers[j] = X[mask].mean(axis=0)
            else:
                # 若某簇无样本，随机重置为某个样本
                idx = rng.integers(0, X.shape[0])
                new_centers[j] = X[idx]
        return new_centers

    def fit(self, X: np.ndarray) -> "KMeans":
        # 初始化聚类中心
        centers = self._init_centers(X)

        # 迭代更新
        for it in range(1, self.max_iter + 1):
            labels = self._assign(X, centers)
            new_centers = self._update_centers(X, labels, centers)

            # 收敛判定：中心移动的最大范数是否小于阈值
            shift = np.max(np.linalg.norm(new_centers - centers, axis=1))
            centers = new_centers
            if shift < self.tol:
                self.n_iter_ = it
                break
            else:
                self.n_iter_ = self.max_iter

        self.cluster_centers_ = centers
        self.labels_ = self._assign(X, centers)
        return self

    def predict(self, X: np.ndarray) -> np.ndarray:
        if self.cluster_centers_ is None:
            raise RuntimeError("请先调用 fit 训练聚类中心。")
        return self._assign(X, self.cluster_centers_)

    def fit_predict(self, X: np.ndarray) -> np.ndarray:
        self.fit(X)
        return self.labels_.copy()


def main():
    parser = argparse.ArgumentParser(description="K-means 聚类（使用文澜特征）")
    parser.add_argument(
        "--npz",
        type=str,
        default=str(
            Path(__file__).resolve().parent / "processed" / "wenlan_embeddings.npz"
        ),
        help="特征文件路径 (.npz)，可选：wenlan_embeddings.npz / wenlan_embeddings_cats.npz / wenlan_embeddings_dogs.npz",
    )
    parser.add_argument("--k", type=int, default=2, help="聚类簇数")
    parser.add_argument("--max-iter", type=int, default=100, help="最大迭代次数")
    parser.add_argument("--tol", type=float, default=1e-4, help="收敛阈值")
    parser.add_argument("--seed", type=int, default=42, help="随机种子")
    parser.add_argument(
        "--run",
        action="store_true",
        help="实际运行 K-means（未实现会抛错，供你填充算法）",
    )

    args = parser.parse_args()

    X, y, paths = load_embeddings(args.npz)
    print(
        f"加载特征成功: X={X.shape}; labels={None if y is None else y.shape}; paths={None if paths is None else paths.shape}"
    )
    X = l2_normalize(X)

    km = KMeans(
        n_clusters=args.k, max_iter=args.max_iter, tol=args.tol, random_state=args.seed
    )
    labels_pred = km.fit_predict(X)
    print(
        f"完成聚类: 迭代次数={km.n_iter_}; 各簇大小={[int(np.sum(labels_pred==i)) for i in range(args.k)]}"
    )

    # 可视化输出目录与命名前缀
    out_dir = Path(__file__).resolve().parent / "outputs"
    out_dir.mkdir(parents=True, exist_ok=True)
    fig_tag = f"{Path(args.npz).stem}_{args.preprocess}_k{args.k}"

    # 1) 显示每一类的图片（三张一行）
    if paths is not None:
        k = int(np.max(labels_pred)) + 1
        for c in range(k):
            idxs = np.where(labels_pred == c)[0]
            if len(idxs) == 0:
                continue

            n = len(idxs)
            cols = 3
            rows = (n + cols - 1) // cols
            plt.figure(figsize=(cols * 3, rows * 3))
            plt.suptitle(f"第{c+1}类（共{n}张）")

            for i, idx in enumerate(idxs, 1):
                img_path = str(paths[idx])
                img = None
                try:
                    from PIL import Image  # 优先使用 PIL

                    img = np.array(Image.open(img_path).convert("RGB"))
                except Exception:
                    try:
                        import matplotlib.image as mpimg  # 退回到 matplotlib 读取

                        img = mpimg.imread(img_path)
                    except Exception as e:
                        print(f"无法读取图片: {img_path}: {e}")
                        continue

                ax = plt.subplot(rows, cols, i)
                ax.imshow(img)
                ax.axis("off")
                ax.set_title(Path(img_path).name, fontsize=8)

            plt.tight_layout(rect=[0, 0, 1, 0.95])
            # 保存该簇的拼图
            save_path = out_dir / f"{fig_tag}_cluster_{c+1:02d}.png"
            try:
                plt.savefig(save_path, dpi=200, bbox_inches="tight")
                print(f"已保存: {save_path}")
            except Exception as e:
                print(f"保存聚类拼图失败: {e}")
            plt.show()
    else:
        print("注意：paths 为 None，无法显示图片。")

    # 2) PCA 降维到 2D 并绘图（直接使用 X 和 labels_pred）
    if X is not None and labels_pred is not None:
        pca = PCA(n_components=2, random_state=42)
        X2d = pca.fit_transform(X)

        plt.figure(figsize=(6, 5))
        scatter = plt.scatter(
            X2d[:, 0], X2d[:, 1], c=labels_pred, cmap="tab10", s=20, alpha=0.8
        )
        # 将聚类中心投影到同一 2D 空间并标出
        try:
            if km.cluster_centers_ is not None:
                centers_2d = pca.transform(km.cluster_centers_)
                plt.scatter(
                    centers_2d[:, 0],
                    centers_2d[:, 1],
                    c="black",
                    s=150,
                    marker="X",
                    edgecolors="white",
                    linewidths=1.5,
                    label="聚类中心",
                )
                plt.legend(loc="best")
        except Exception as e:
            print(f"中心可视化时出错（忽略不影响主流程）：{e}")
        plt.title("K-means(PCA 2D)")
        plt.xlabel("PC1")
        plt.ylabel("PC2")
        plt.colorbar(scatter, label="Cluster")
        plt.tight_layout()
        # 保存 PCA 2D 图
        pca_save_path = out_dir / f"{fig_tag}_pca2d.png"
        try:
            plt.savefig(pca_save_path, dpi=200, bbox_inches="tight")
            print(f"已保存: {pca_save_path}")
        except Exception as e:
            print(f"保存 PCA 图失败: {e}")
        plt.show()
    else:
        print("注意：未找到 X 或 labels_pred，无法进行可视化。")


if __name__ == "__main__":
    main()

# 对猫/狗数据做聚类
# python .\K-means.py --npz .\processed\wenlan_embeddings_cats.npz --k 4 --run
# python .\K-means.py --npz .\processed\wenlan_embeddings_dogs.npz --k 4 --run