import numpy as np
import matplotlib.pyplot as plt
from scipy.spatial.distance import pdist

# 设置中文字体 (如果系统没有SimHei，可能会显示方框，可以改用英文)
plt.rcParams["font.sans-serif"] = ["SimHei"]
plt.rcParams["axes.unicode_minus"] = False

# 设置参数
dim = 100
n_samples = 30

# 1. 生成数据: [-0.5, 0.5]^100 中的均匀分布
# 形状为 (30, 100)
X = np.random.uniform(-0.5, 0.5, (n_samples, dim))

# 2. 计算所有点对之间的距离
# pdist 计算成对欧氏距离
distances = pdist(X, metric="euclidean")

# 3. 计算所有点向量之间的夹角
# 先归一化向量，使其模长为1
norms = np.linalg.norm(X, axis=1, keepdims=True)
X_normalized = X / norms

# 计算余弦相似度矩阵: X_norm * X_norm^T
# 结果矩阵的 (i, j) 元素就是点 i 和点 j 的余弦值
cosine_similarity = np.dot(X_normalized, X_normalized.T)

# 提取上三角部分的余弦值（不包含对角线，因为对角线是自己和自己的夹角0）
# np.triu_indices(n, k=1) 返回上三角的索引，k=1表示不含对角线
indices = np.triu_indices(n_samples, k=1)
cos_thetas = cosine_similarity[indices]

# 截断数值以防浮点误差导致超出 [-1, 1]
cos_thetas = np.clip(cos_thetas, -1.0, 1.0)

# 计算角度 (弧度 -> 角度)
angles = np.arccos(cos_thetas) * (180 / np.pi)

# 4. 绘图
plt.figure(figsize=(12, 5))

# 距离直方图
plt.subplot(1, 2, 1)
plt.hist(distances, bins=15, color="skyblue", edgecolor="black", alpha=0.7)
plt.title(f"100维空间中随机点对的距离分布")
plt.xlabel("欧氏距离")
plt.ylabel("频数")

# 角度直方图
plt.subplot(1, 2, 2)
plt.hist(angles, bins=15, color="salmon", edgecolor="black", alpha=0.7)
plt.title(f"100维空间中随机向量的夹角分布")
plt.xlabel("角度 (度)")
plt.ylabel("频数")

plt.tight_layout()

# 保存图片
save_path = "high_dim_plot.png"
plt.savefig(save_path, dpi=300)
print(f"图片已保存为: {save_path}")