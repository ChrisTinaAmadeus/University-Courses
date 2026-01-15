import time
import matplotlib.pyplot as plt
import math
import numpy as np
import warnings
warnings.filterwarnings('ignore')
plt.rcParams["font.sans-serif"] = ["SimHei"]  # 设置中文字体
plt.rcParams["axes.unicode_minus"] = False  # 解决负号显示问题

start, end, step = 10_000, 1_000_000, 10_000
sizes = list(range(start, end + 1, step))  # 10000 到 1000000
append_times = []
insert_times = []

for n in sizes:
    # append
    lst = []
    t0 = time.time()
    for i in range(n):
        lst.append(i)
    append_times.append(time.time() - t0)

    # insert(0, x)
    lst = []
    t0 = time.time()
    for i in range(n):
        lst.insert(0, i)
    insert_times.append(time.time() - t0)

plt.figure(figsize=(10,6))
plt.plot(sizes, append_times, marker='o', label='append')
plt.plot(sizes, insert_times, marker='o', label='insert(0, x)')
plt.xlabel('插入数据量')
plt.ylabel('运行时间 (秒)')
plt.title('append 和 insert(0, x) 时间比较')
plt.legend()
plt.grid(True)
plt.show()