# 🎓 University-Courses

> 中国人民大学（RUC）本科专业课学习资料存档，涵盖计算机系统、人工智能、数据结构、算法设计等多个方向。

---

## 📖 目录

- [🎓 University-Courses](#-university-courses)
  - [📖 目录](#-目录)
  - [仓库简介](#仓库简介)
  - [课程目录](#课程目录)
    - [ICS · 计算机系统导论](#ics--计算机系统导论)
    - [人工智能引论](#人工智能引论)
    - [人工智能数学基础](#人工智能数学基础)
    - [人工智能与 Python 程序设计](#人工智能与-python-程序设计)
    - [人工智能伦理与安全](#人工智能伦理与安全)
    - [人工智能综合设计](#人工智能综合设计)
    - [机器学习](#机器学习)
    - [数据结构与算法](#数据结构与算法)
    - [算法设计与分析](#算法设计与分析)
    - [最优化理论与方法](#最优化理论与方法)
    - [程序设计](#程序设计)
  - [联系方式](#联系方式)

---

## 仓库简介

本仓库整理了我本科期间修读的专业课程相关资料，包括**作业、实验代码、课程笔记、试卷与复习资料**等。所有内容按课程分类，方便检索与复习。

---

## 课程目录

### ICS · 计算机系统导论

```
ICS/
├── note/                          # 课程笔记（.docx）
│   ├── 01-总述.docx               #   课程总览
│   ├── 02-位.docx                 #   位运算与数据表示
│   ├── 03-汇编.docx               #   汇编语言
│   ├── 04-流水线.docx             #   处理器流水线
│   ├── 06-存储.docx               #   存储器层次结构
│   ├── 07-链接.docx               #   链接
│   ├── 08-控制流.docx             #   异常控制流
│   ├── 09-虚拟内存.docx           #   虚拟内存
│   ├── 10-IO.docx                 #   I/O 系统
│   ├── 11-FS.docx                 #   文件系统
│   └── 12-线程.docx               #   线程与并发
├── homework/                      # 课后作业（LaTeX / Markdown）
│   ├── 位运算1/ ~ 位运算4/        #   位运算与浮点数
│   ├── 汇编1/ ~ 汇编4/            #   汇编基础与控制流
│   ├── 存储1/ ~ 存储2/            #   存储器
│   ├── 异常1/ ~ 异常3/            #   异常控制流
│   ├── 虚存1/ ~ 虚存4/            #   虚拟内存
│   ├── FS/                        #   文件系统
│   ├── IO/                        #   I/O
│   └── 线程/                      #   线程
└── lab/                           # 课程实验
    ├── data lab/                  #   数据表示实验（bits.c）
    ├── bomb lab/                  #   二进制炸弹拆解
    ├── attack lab/                #   缓冲区溢出攻击
    ├── cache lab/                 #   缓存模拟器
    ├── link lab/                  #   链接器实验（ld.cpp, nm.cpp）
    ├── malloclab/                 #   动态内存分配器
    ├── locklab/                   #   并发锁竞争分析
    ├── tmuxlab/                   #   简易终端复用器（mini_tmux.cpp）
    └── colab/                     #   协同调度器（scheduler.cc）
```

### 人工智能引论

```
人工智能引论/
├── lab/
│   ├── A星算法/                   # A* 搜索
│   ├── α-β剪枝/                   # 博弈树搜索
│   ├── 蒙特卡洛/                   # 蒙特卡洛方法
│   ├── K-means/                   # K-means 聚类
│   ├── 强化学习/                   # Q-Learning
│   ├── 词向量/                     # Word2Vec
│   ├── Transformer/               # Transformer 机器翻译
│   ├── 图像分类/                   # CNN 图像分类
│   ├── 语音特征提取/               # MFCC 特征提取
│   └── 语音合成/                   # TTS 语音合成
├── 期末复习.pdf
├── 期末复习1.pdf
└── 期末复习2.pdf
```

### 人工智能数学基础

```
人工智能数学基础/
├── 作业/
│   ├── 1/                         # 代数系统
│   ├── 2/ ~ 5/                    # 线性代数
│   ├── 6/                         # QR 分解（含代码）
│   ├── 7/                         # 微分与导数
│   ├── 8/                         # 优化（含图像）
│   ├── 9/                         # 算法
│   ├── 10/                        # 概率基础
│   └── 11/                        # 高维概率（含可视化代码）
├── 24级人工智能数学基础期末考试题.pdf
└── 人工智能数学基础复习.pdf
```

### 人工智能与 Python 程序设计

```
人工智能与Python程序设计/
├── PyTorch简介.ipynb / PyTorch简介2.ipynb   # PyTorch 入门
├── Numpy库.ipynb                             # NumPy 基础
├── Matplotlib.ipynb                          # Matplotlib 绘图
├── Parameter基础.ipynb                        # 自动求导
├── 面向对象.ipynb / 面向对象总结.ipynb         # OOP
├── 一元线性回归.py                            # 线性回归
├── 多元线性回归np版.py / t+parameter版.py      # 多元回归（多种实现）
├── Numpy逻辑回归.ipynb                        # 逻辑回归（NumPy）
├── Pytorch实现逻辑回归.ipynb                   # 逻辑回归（PyTorch）
├── MLP多层感知机.ipynb                        # 多层感知机
├── 图像与卷积.ipynb                           # 卷积基础
├── 卷积神经网络介绍.ipynb                      # CNN 介绍
├── 简单卷积神经网络分类模型的训练和测试 - 完整版.ipynb
├── 简单卷积神经网络分类模型的训练和测试 - 逐步骤讲解.ipynb
├── CCN模型创建与训练.py                       # CCN 模型
└── 随机点名.py                                # 趣味小工具
```

### 人工智能伦理与安全

```
人工智能伦理与安全/
├── 2024201594-王松宸-期中论文.docx
└── 2024201594-王松宸-期末论文.tex
```

### 人工智能综合设计

```
人工智能综合设计/
├── src/
│   ├── app.py                    # Flask 应用入口
│   ├── search_engine.py          # 搜索引擎核心
│   ├── search_engine_UI.py       # 搜索 UI 逻辑
│   ├── client.py                 # 客户端
│   ├── get_url.py                # URL 抓取
│   ├── save_data.py              # 数据持久化
│   ├── templates/                # HTML 模板
│   │   ├── index.html
│   │   └── results.html
│   ├── static/                   # 静态资源
│   │   ├── style.css
│   │   └── 天空.png
│   └── *.pkl / *.txt             # 倒排索引、停用词表等数据文件
├── report.pdf / report.tex       # 课程报告
└── PPT.pptx                      # 答辩展示
```

### 机器学习

```
机器学习/
├── homework/
│   ├── HW1/
│   │   ├── Lab1/                 # 线性回归
│   │   ├── Lab2/                 # 逻辑回归
│   │   └── report.pdf            # HW1 实验报告
│   ├── HW2/
│   │   ├── Lab3/                 # SVM
│   │   ├── Lab4/                 # 集成学习
│   │   └── report.pdf
│   ├── HW3/
│   │   ├── Lab5/                 # 聚类
│   │   ├── Lab6/                 # PCA 降维
│   │   └── report.pdf
│   ├── HW4/
│   │   ├── Lab7/                 # 贝叶斯与 EM 算法
│   │   ├── Lab8/                 # HMM
│   │   └── report.pdf
│   └── HW5/
│       ├── Lab9/                 # 决策树
│       ├── Lab10/                # 神经网络（adult 数据集）
│       └── report.pdf
├── ML（Amadeus）.pdf              # 个人复习笔记
├── 宁哥笔记.pdf                   # 同学笔记
├── 期末样题.pdf
└── 样题答案.pdf
```

### 数据结构与算法

```
数据结构与算法/
├── lab1/                         # 字符串操作
│   ├── lab1.cpp
│   ├── report/
│   └── 要求.md
├── lab2/                         # HTML 结构解析与校验
│   ├── lab2.cpp
│   ├── test_cases/
│   ├── report/
│   └── 要求.md
├── lab3/                         # HTML Selector（课程设计）
│   ├── lab3.cpp
│   ├── test_data/
│   ├── report/
│   │   ├── 使用手册/
│   │   ├── 功能测试报告/
│   │   └── 实验报告/
│   └── examples/
├── lab4/                         # 最短路径中文分词
│   ├── lab4.cpp
│   ├── dict.txt / dict_big.txt
│   ├── report/
│   │   ├── 使用手册/
│   │   ├── 功能测试报告/
│   │   └── 实验报告/
│   └── 要求.md
├── 作业/
│   ├── 1/                        # append vs insert 性能分析（含代码与图像）
│   ├── 2/                        # 第 1 章习题
│   ├── 3/                        # 第 2 章习题
│   ├── 4/                        # 第 3-4 章习题
│   └── 5/                        # 第 6 章习题
├── 期中.jpg                       # 期中手写笔记
└── 期末-*.jpg                     # 期末知识点整理（树/图/查找/排序）
```

### 算法设计与分析

```
算法设计与分析/
├── homework/
│   ├── 2024201594-王松宸-作业1~11.pdf    # 课后作业
│   └── 2024201594-王松宸-思考题1~11.pdf  # 思考题
├── 2020级算法期中试题.pdf / 答案.pdf
├── 2023级算法期中试题.pdf
├── 2023级算法期末试题.pdf
├── 算法（Amadeus）.pdf                   # 个人复习笔记
└── 算法试题（Amadeus）.pdf               # 个人整理试题
```

### 最优化理论与方法

```
最优化理论与方法/
├── 1/  homework1.pdf + answer.pdf    # 凸集与凸函数
├── 2/  homework2.pdf + answer.pdf    # 线性规划
├── 3/  homework3.pdf + answer.pdf    # 对偶理论
├── 4/  homework4.pdf + answer.pdf    # 无约束优化
├── 5/  homework5.pdf + answer.pdf    # 约束优化
└── 6/  homework6-kkt.pdf + homework6.pdf + KKT example.pdf  # 非线性优化和 KKT 条件
```

### 程序设计

```
程序设计/
├── README.md              # 该课程详细说明（含题目标签）
├── helloworld.cpp         # 你好，世界！
├── ASCII.cpp              # ASCII 码转换
├── fibonacci.cpp          # 斐波那契数列
├── hannuota.cpp           # 汉诺塔
├── huiwen.cpp             # 回文判断
├── sushu.cpp              # 素数
├── guibingsort.cpp        # 归并排序
├── quicksort.cpp          # 快速排序
├── nqueen.cpp             # N 皇后问题
├── maze.cpp               # 迷宫搜索
├── link.cpp               # 链表操作
├── package.cpp            # 背包问题
├── ...                    # 共 200+ 道练习题目
└── (更多 C++ 源文件)
```

---

## 联系方式

📧 [wangsongchen@ruc.edu.cn](mailto:wangsongchen@ruc.edu.cn)

欢迎邮件联系与交流。

---

<p align="center">
  <sub>Made with ❤️ at RUC</sub>
</p>
