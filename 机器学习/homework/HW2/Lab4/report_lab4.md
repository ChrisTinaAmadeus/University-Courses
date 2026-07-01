# Lab 5：矩阵分解与线性降维（实现说明）

> 对应代码：`Lab4/script_lab4.py`（Python ≥ 3.6，仅用 numpy/matplotlib）

本实验围绕两类核心思想：
1) **二阶统计（协方差）驱动的线性方法**：PCA、白化、以及“协方差投毒”。
2) **带结构约束的矩阵分解**：鲁棒 PCA（低秩 + 稀疏）与 NMF（非负分解）。

为了符合“偏数学”的实现要求，代码里尽量用 **显式的矩阵运算**（均值、协方差、特征分解、投影、逐元素更新等），避免直接调用一步到位的高层黑盒（例如直接 SVD 解完整问题）。

---

## Task 1：PCA（特征分解实现）

### 1.1 目标与优化视角
给定数据矩阵 $X\in\mathbb{R}^{N\times D}$（每行一个样本），PCA 的经典目标是找到一个 $k$ 维子空间（$k=\text{n\_pc}$），使得把数据投影到该子空间后，**重构误差最小**：

$$
\min_{V\in\mathbb{R}^{D\times k},\;V^TV=I}\;\|X_0 - X_0VV^T\|_F^2
$$

其中 $X_0 = X - \mathbf{1}\mu^T$ 是中心化后的数据，$\mu=\frac{1}{N}\sum_i x_i$。

等价地，它也最大化投影后方差：

$$
\max_{V^TV=I}\;\mathrm{tr}(V^T\Sigma V),\quad \Sigma=\frac{1}{N}X_0^TX_0
$$

### 1.2 特征分解与主成分
协方差矩阵 $\Sigma$ 是对称半正定矩阵，可做特征分解：

$$
\Sigma = Q\Lambda Q^T
$$

- $Q=[q_1,\dots,q_D]$：特征向量（正交基）
- $\Lambda=\mathrm{diag}(\lambda_1,\dots,\lambda_D)$：特征值（方差大小）
- 将 $\lambda$ 从大到小排序，取前 $k$ 个特征向量 $V=[q_1,\dots,q_k]$ 就是主成分方向。

### 1.3 投影与重构
- 低维表示（主成分坐标）：$Z = X_0V\in\mathbb{R}^{N\times k}$
- 在零均值空间重构：$\hat X_0 = ZV^T = X_0VV^T$

代码对应：
- 先显式中心化 `xs0 = xs - mean(xs)`
- 用 `cov = xs0.T @ xs0 / N` 得到协方差
- 用 `np.linalg.eigh(cov)` 做对称矩阵特征分解并排序
- `xhat0 = (xs0 @ vs) @ vs.T` 得到重构

> 注意：特征向量符号不唯一（$q$ 与 $-q$ 都是同一方向），因此画出来的主方向可能翻转，但几何意义不变。

---

## Task 2：数据白化（Whitening）

### 2.1 白化要做什么？
白化希望找到线性变换 $W$，使得对中心化数据 $X_0$：

$$
Y = X_0 W
$$

满足

$$
\mathbb{E}[Y]=0,\quad \mathrm{Cov}(Y)=I
$$

即：
- 均值为 0
- 各维不相关（协方差为对角）且方差都为 1（协方差为单位阵）

如果原数据是高斯分布，那么“去相关 + 单位方差”会把它变成标准正态；如果不是高斯，白化至少保证二阶统计被标准化（不承诺分布真的变高斯）。

### 2.2 Lecture 2 方法：协方差的逆平方根
设 $\Sigma=\frac{1}{N}X_0^TX_0$，若

$$
\Sigma = Q\Lambda Q^T
$$

则

$$
\Sigma^{-1/2}=Q\Lambda^{-1/2}Q^T
$$

令 $W=\Sigma^{-1/2}$ 就能得到 $\mathrm{Cov}(X_0W)=I$。

### 2.3 Lecture 5（PCA）方法：先旋转再缩放
PCA 给出把数据旋转到主轴的正交矩阵 $Q$：

$$
Z = X_0Q
$$

此时

$$
\mathrm{Cov}(Z)=\Lambda
$$

再对每一维做缩放：

$$
Y_{\text{PCA}} = Z\Lambda^{-1/2}
$$

就得到单位协方差。

### 2.4 PCA 与白化的关系（核心联结）
从上面可以看到：
- **PCA 的特征分解把协方差“对角化”**（去相关）
- **白化是在 PCA 的基础上再做“按特征值归一化”**（把方差变成 1）

因此白化可以理解为：

> “PCA 旋转（decorrelate） + 方差归一（rescale）”

结合优势：
- 许多学习算法（梯度下降、最小二乘、独立成分分析 ICA 等）在各维尺度差异很大、且强相关时会变得**病态**（条件数差、收敛慢）。
- 白化把二阶统计标准化后，往往能让优化更稳定，超参数（如学习率）更好调。

### 2.5 代码选择：ZCA Whitening（旋回原坐标系）
白化有两种常见输出：
- **PCA Whitening**：输出 $Y_{\text{PCA}}$（在主轴坐标系里）
- **ZCA Whitening**：再旋回原坐标系：

$$
Y_{\text{ZCA}} = Y_{\text{PCA}}Q^T = X_0Q\Lambda^{-1/2}Q^T
$$

ZCA 的优点是：输出仍在“原坐标系”里，视觉上更容易和原数据对照。

实现里使用了 `pca(xs, n_pc=D)` 得到 $Q$ 与 $\lambda$，并加了很小的 `eps` 防止 $\lambda\approx 0$ 时数值爆炸。

---

## Task 3：鲁棒 PCA（Hard-threshold Alternating）

### 3.1 模型：低秩 + 稀疏（outlier）
鲁棒 PCA 的经典假设是：

$$
X_0 = L + S
$$

- $L$：低秩（“干净结构”，例如落在低维子空间的主趋势）
- $S$：稀疏（“异常/离群”，只在少数位置出现大扰动）

本 lab 采用的目标是：

$$
\min_{L,S}\;\|X_0-(L+S)\|_F^2
\quad\text{s.t.}\quad \mathrm{rank}(L)\le k,\; \|S\|_0 \le \rho ND
$$

其中 $\|S\|_0$ 计数非零元素个数，$\rho=\text{ratio\_nz}$。

### 3.2 交替优化（Alternating Minimization）
因为两个约束同时处理很难，我们用交替法：

**(A) 固定 $S$，更新 $L$**

$$
L \leftarrow \arg\min_{\mathrm{rank}(L)\le k}\;\| (X_0-S) - L\|_F^2
$$

这一步的解是：对矩阵 $A=X_0-S$ 做“最佳 rank-$k$ 逼近”。理论上它来自截断 SVD；但为了满足“用常见 numpy 运算”的要求，实现里用：

- 先算 Gram 矩阵 $G=A^TA\in\mathbb{R}^{D\times D}$
- 对称特征分解 $G=V\Gamma V^T$
- 取前 $k$ 个特征向量 $V_k$
- 投影得到低秩部分：

$$
L = A V_k V_k^T
$$

**(B) 固定 $L$，更新 $S$**

$$
S \leftarrow \arg\min_{\|S\|_0\le \rho ND}\;\| (X_0-L) - S\|_F^2
$$

令残差 $R=X_0-L$，要让 $\|R-S\|_F^2$ 最小且 $S$ 稀疏，最优策略是：

> 让 $S$ 只保留 $R$ 中绝对值最大的那一小部分元素，其余置 0。

这就是 hard-thresholding（硬阈值）算子。

### 3.3 hard-thresholding 的自适应阈值
实现里不是手写“阈值数值”，而是：
- 设总元素数 $M=ND$
- 取 $k=\lfloor \rho M\rfloor$
- 用 `np.argpartition` 找到绝对值最大的 top-$k$ 的索引
- 只保留这些位置，其余置 0

这样能保证稀疏度受控，且时间复杂度接近 $O(M)$（比完整排序更省）。

### 3.4 输出如何理解
最终返回：
- `xhat0 = low_rank`：鲁棒重构的“干净数据”（离群点影响被稀疏项吸收）
- `vs, lambdas`：对低秩部分做协方差特征分解得到的主方向与方差

---

## Task 4：耦合离群点投毒（Coupled Outlier Poisoning）

### 4.1 约束条件
需要构造两点 $x_1,x_2\in\mathbb{R}^D$ 满足：

$$
\|x_1\|_2=\|x_2\|_2=1,\quad x_1+x_2=0
$$

因此必有 $x_2=-x_1$。这组“正负对称”点的一个重要性质：
- 在中心化空间里，它们对**均值的贡献抵消**，主要影响协方差。

### 4.2 协方差被如何改变？
设中心化后数据为 $X_0$，原协方差：

$$
\Sigma = \frac{1}{N}X_0^TX_0
$$

加入 $\pm x$（$\|x\|=1$）后，新协方差近似为：

$$
\Sigma' = \frac{1}{N+2}\big(X_0^TX_0 + x x^T + (-x)(-x)^T\big)
= \frac{1}{N+2}\big(X_0^TX_0 + 2xx^T\big)
$$

其中 $xx^T$ 是一个 rank-1 的“方差注入”项，方向就是 $x$。

### 4.3 为什么选“最小特征值方向”？（直觉 + 数学）
在预算固定（$\|x\|=1$）时，想让协方差变化尽量大，一个有效策略是：

> 把方差注入到数据原本方差最小的方向（即最小特征值对应的特征向量）。

直觉：
- 数据原本“几乎不往那边扩散”，你往那边强行加两个单位范数点，会让协方差结构发生更明显的扭转，主成分方向更容易被拉偏。

实现：
- 对原协方差做特征分解
- 取最小特征值对应特征向量 $v_{\min}$
- 设 $x_1=v_{\min}$，$x_2=-v_{\min}$

---

## Task 5：非负矩阵分解（NMF）

### 5.1 目标函数
给定非负数据 $X\ge 0$（本实验用 `rand` 生成天然非负），NMF 目标：

$$
\min_{U\ge 0,\;V\ge 0}\;\|X-UV^T\|_F^2
$$

其中 $U\in\mathbb{R}_+^{N\times r}$，$V\in\mathbb{R}_+^{D\times r}$。

NMF 与 PCA 的最大区别：
- PCA 子空间基向量允许正负，重构是“正负抵消”的线性组合
- NMF 强制非负，常常得到更“可解释”的部件（parts-based）表示（例如图像中的局部结构）

### 5.2 乘法更新（Multiplicative Updates）
经典的 Lee & Seung 更新在 Frobenius 范数下为：

$$
U \leftarrow U \odot \frac{XV}{U(V^TV)}
$$

$$
V \leftarrow V \odot \frac{X^TU}{V(U^TU)}
$$

其中 $\odot$、分式都是逐元素运算。

关键性质：
- 只要初始化 $U,V$ 非负，乘法更新会保持非负
- 每次更新只用矩阵乘法与逐元素运算，易实现

实现细节：
- 分母加 `eps` 防止除 0
- 每轮更新后用 `np.maximum(·,0)` 做数值安全夹断

---

## 如何运行与输出
在 `Lab4/` 目录运行：

```bash
python script_lab4.py
```

会生成：
- `result_gauss.png`、`result_outlier.png`：PCA vs RPCA 的点云与主方向对比
- `whitening_gauss.png`、`whitening_outlier.png`：白化前后对比
- `poisoning_pca.png`：投毒前后 PCA 主方向变化

---

## 参考（可选）
- [1] I. T. Jolliffe, *Principal Component Analysis*.
- [2] A. Hyvärinen, J. Karhunen, E. Oja, *Independent Component Analysis*（白化与 ICA 的关系讲得很清楚）.
- [3] D. Lee, H. Seung, “Algorithms for Non-negative Matrix Factorization”, 2001.
