import numpy as np

def Monte_Carlo(n):
    # 生成n个[0,1)区间的随机点
    x = np.random.rand(n)
    y = np.random.rand(n)

    # 判断点是否在单位圆内
    is_in_circle = x**2 + y**2 <= 1

    # 计算圆周率估计值
    pi_estimate = 4 * np.sum(is_in_circle) / n
    print(f"估算的π值: {pi_estimate}")

for n in [1000,10000,100000,1000000,10000000,100000000,500000000]:
    print('点的数量为',n,'时，')
    Monte_Carlo(n)