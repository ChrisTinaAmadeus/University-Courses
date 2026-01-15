import numpy as np
import random
import heapq
import math


class PuzzleBoardState(object):
    """华容道棋盘类（支持任意阶 dim×dim）"""

    def __init__(self, dim=3, random_seed=2022, data=None, parent=None):
        """
        dim         : int, 棋盘维度(阶数)
        random_seed : int, 创建随机棋盘的随机种子
        data        : np.ndarray (dim*dim), 初始棋盘（未显式做可解性检查）
        parent      : PuzzleBoardState, 父节点（用于回溯路径）
        """
        self.dim = dim
        # 目标终局：1..(n^2-1), 0
        self.default_dst_data = self._goal_board(dim)

        if data is None:
            init_solvable = False
            init_count = 0
            while not init_solvable and init_count < 500:
                init_data = self._get_random_data(random_seed=random_seed + init_count)
                init_count += 1
                init_solvable = self._if_solvable(init_data, self.default_dst_data)
            data = init_data
        self.data = data
        self.parent = parent
        self.piece_x, self.piece_y = self._get_piece_index()

    @staticmethod
    def _goal_board(dim: int) -> np.ndarray:
        arr = list(range(1, dim * dim)) + [0]
        return np.array(arr, dtype=int).reshape(dim, dim)

    def _get_random_data(self, random_seed):
        """生成 dim×dim 的随机棋盘"""
        random.seed(random_seed)
        init_data = [i for i in range(self.dim ** 2)]
        random.shuffle(init_data)
        init_data = np.array(init_data).reshape((self.dim, self.dim))
        return init_data

    def _get_piece_index(self):
        """返回当前空格(0)的位置 (x, y)"""
        x, y = np.argwhere(self.data == 0)[0]
        return int(x), int(y)

    def _inverse_num(self, puzzle_board_data):
        """计算一维展开后的逆序数（忽略0）"""
        flatten_data = puzzle_board_data.flatten()
        res = 0
        for i in range(len(flatten_data)):
            if flatten_data[i] == 0:
                continue
            for j in range(i):
                if flatten_data[j] > flatten_data[i] and flatten_data[j] != 0:
                    res += 1
        return res

    def _if_solvable(self, src_data, dst_data):
        """
        判断 (src_data -> dst_data) 是否可解（仅适用于奇数阶 8-puzzle 常见规则）
        更一般的判定需考虑空格所在行等，这里保持与原实现一致（逆序和是否同奇偶）
        """
        assert src_data.shape == dst_data.shape, "src_data and dst_data should share same shape."
        inverse_num_sum = self._inverse_num(src_data) + self._inverse_num(dst_data)
        return inverse_num_sum % 2 == 0

    def is_final(self):
        """是否到达目标终止状态"""
        return np.array_equal(self.data, self.default_dst_data)

    def next_states(self):
        """返回当前状态的相邻状态（4 邻域：上下左右移动空格）"""
        res = []
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            x2, y2 = self.piece_x + dx, self.piece_y + dy
            if 0 <= x2 < self.dim and 0 <= y2 < self.dim:
                new_data = self.data.copy()
                # 交换空格与目标格
                new_data[self.piece_x][self.piece_y] = new_data[x2][y2]
                new_data[x2][y2] = 0
                # 关键：生成子状态时记录 parent=self，便于回溯路径
                res.append(PuzzleBoardState(dim=self.dim, data=new_data, parent=self))
        return res

    def get_data(self):
        return self.data

    def get_data_hash(self):
        """将棋盘数据转为可哈希的 tuple 形式以便存入集合/字典"""
        return hash(tuple(self.data.flatten()))

    def get_parent(self):
        return self.parent


# -------------------- 参考：BFS（保持你的原实现） --------------------
def bfs(puzzle_board_state):
    visited = set()
    from collections import deque
    queue = deque()
    queue.append((0, puzzle_board_state))
    visited.add(puzzle_board_state.get_data_hash())

    ans = []
    while queue:
        (now, cur_state) = queue.popleft()
        if cur_state.is_final():
            while cur_state.get_parent() is not None:
                ans.append(cur_state)
                cur_state = cur_state.get_parent()
            ans.append(cur_state)
            break

        next_states = cur_state.next_states()
        for next_state in next_states:
            if next_state.get_data_hash() in visited:
                continue
            visited.add(next_state.get_data_hash())
            queue.append((now + 1, next_state))

    return ans


# -------------------- 启发式函数：曼哈顿距离 --------------------
def heuristic(state: PuzzleBoardState) -> int:
    """
    计算每个非零方块与其目标位置的曼哈顿距离之和
    """
    h = 0
    for i in range(state.dim):
        for j in range(state.dim):
            value = state.data[i][j]
            if value == 0:
                continue
            target_i, target_j = divmod(value - 1, state.dim)
            h += abs(i - target_i) + abs(j - target_j)
    return h


# -------------------- A*：严格对应你的伪代码 1~8 步 --------------------
def astar(start_state: PuzzleBoardState):
    """
    A* 搜索（OPEN: 最小堆; CLOSED: 已确定最优g的集合）
    对应伪代码：
      1) OPEN:=(s), f(s):=g(s)+h(s)
      2) 循环直到 OPEN 为空
      3) n := FIRST(OPEN)    # 取 f 最小
      4) 若 n 为目标则成功
      5) 从 OPEN 移除 n，加入 CLOSED
      6) 扩展 n 的子节点并按三种情况松弛/入队
      7) OPEN 由堆自动按 f 排序
      8) 回到循环
    """
    # 1) 初始化
    start_hash = start_state.get_data_hash()
    g_score = {start_hash: 0}                         # g(s) = 0
    f_score = {start_hash: heuristic(start_state)}    # f(s) = g(s) + h(s)

    open_heap = []                                    # OPEN：按 f 值的小根堆
    counter = 0                                       # 打破同 f 的比较歧义
    heapq.heappush(open_heap, (f_score[start_hash], g_score[start_hash], counter, start_state))
    counter += 1

    closed = set()                                    # CLOSED：已“确定最优”的节点集合

    # 2) 主循环
    while open_heap:
        # 3) n := FIRST(OPEN) —— 取 f 最小
        f, g, _, current = heapq.heappop(open_heap)
        cur_hash = current.get_data_hash()

        # 堆里可能有“过期条目”（旧的更差路径），用 g_score 过滤
        if g != g_score.get(cur_hash, math.inf):
            continue

        # 4) 目标检测
        if current.is_final():
            # 回溯出从起点到终点的路径
            path = []
            node = current
            while node is not None:
                path.append(node)
                node = node.get_parent()
            return path[::-1]

        # 5) 移出 OPEN，加入 CLOSED
        closed.add(cur_hash)

        # 6) 扩展 n -> {m_i}
        for neighbor in current.next_states():
            n_hash = neighbor.get_data_hash()
            tentative_g = g + 1  # 每移动一步代价+1：g(n, m_i) = g(n) + cost(n->m_i)=1

            # 6.1 若在 CLOSED 且新路径不更优，跳过
            if n_hash in closed and tentative_g >= g_score.get(n_hash, math.inf):
                continue

            # 6.2 若找到更优路径（或首次发现该节点）
            if tentative_g < g_score.get(n_hash, math.inf):
                g_score[n_hash] = tentative_g
                f_new = tentative_g + heuristic(neighbor)  # f(n, m_i) = g(n, m_i) + h(m_i)
                f_score[n_hash] = f_new

                # “标记 m 到 n 的指针”——在生成 neighbor 时已设置 parent=self（current）
                # 若多次发现更优路径，neighbor.parent 会随“更优版本的对象”更新

                # 6.3 将 m_i 加入 OPEN（等价于伪码中的 ADD）
                heapq.heappush(open_heap, (f_new, tentative_g, counter, neighbor))
                counter += 1

        # 7) OPEN 自动按 f 排序（heapq 维护），无需手动 sort

    # 2) 若 OPEN 为空，失败
    return None


# -------------------- 命令行交互 --------------------
# if __name__ == "__main__":
#     dim = int(input("请输入棋盘维度(阶数): "))
#     initial_data = []
#     print("请输入初始棋盘状态，每行数字之间用空格分隔:")
#     for _ in range(dim):
#         row = list(map(int, input().split()))
#         initial_data.append(row)

#     test_data = np.array(initial_data, dtype=int)
#     test_board = PuzzleBoardState(dim=dim, data=test_data)

#     method = input("请选择搜索方法 (bfs 或 astar): ").strip().lower()
#     if method == 'bfs':
#         result = bfs(test_board)
#         # BFS 返回的是从终点回溯到起点的序列（已在函数内倒回一次）
#         # 这里直接按你原来的打印方式再反转一次
#         if result:
#             print("找到解决方案！（BFS）")
#             for i in reversed(result):
#                 print(i.data)
#         else:
#             print("没有找到解决方案.")
#     elif method == 'astar':
#         result = astar(test_board)
#         if result:
#             print("找到解决方案！（A*）")
#             # A* 的 astar() 已返回起点->终点的正向路径
#             for s in result:
#                 print(s.data)
#         else:
#             print("没有找到解决方案.")
#     else:
#         print("无效的输入，请输入 'bfs' 或 'astar'.")


import time

def bfs_with_stats(start_state):
    from collections import deque
    visited = set()
    q = deque()
    q.append(start_state)
    visited.add(start_state.get_data_hash())
    expansions = 0

    while q:
        cur = q.popleft()
        if cur.is_final():
            # 回溯路径长度
            path = []
            x = cur
            while x is not None:
                path.append(x)
                x = x.get_parent()
            return path[::-1], {"expansions": expansions}
        expansions += 1
        for nxt in cur.next_states():
            h = nxt.get_data_hash()
            if h not in visited:
                visited.add(h)
                q.append(nxt)
    return None, {"expansions": expansions}

def astar_with_stats(start_state):
    path = astar(start_state)  # 直接用你之前接入的 A* 实现
    # 为了统计 expansions，最简便是轻量改造 astar；这里给一个近似统计法：
    # 复用一份带计数器的 A*，不改变原函数：
    import heapq, math
    g_score = {start_state.get_data_hash(): 0}
    open_heap = [(heuristic(start_state), 0, 0, start_state)]
    closed = set()
    counter = 1
    expansions = 0

    while open_heap:
        f, g, _, current = heapq.heappop(open_heap)
        cur_hash = current.get_data_hash()
        if g != g_score.get(cur_hash, math.inf):
            continue
        if current.is_final():
            path = []
            x = current
            while x is not None:
                path.append(x)
                x = x.get_parent()
            return path[::-1], {"expansions": expansions}
        closed.add(cur_hash)
        expansions += 1
        for nxt in current.next_states():
            nh = nxt.get_data_hash()
            tentative_g = g + 1
            if nh in closed and tentative_g >= g_score.get(nh, math.inf):
                continue
            if tentative_g < g_score.get(nh, math.inf):
                g_score[nh] = tentative_g
                f_new = tentative_g + heuristic(nxt)
                heapq.heappush(open_heap, (f_new, tentative_g, counter, nxt))
                counter += 1
    return None, {"expansions": expansions}

def run_case(name, dim, arr):
    print(f"\n=== {name} ===")
    init = np.array(arr).reshape(dim, dim)
    s = PuzzleBoardState(dim=dim, data=init)

    # BFS
    t0 = time.perf_counter()
    bfs_path, bfs_stat = bfs_with_stats(PuzzleBoardState(dim=dim, data=init))
    t1 = time.perf_counter()
    if bfs_path:
        print(f"BFS: steps={len(bfs_path)-1}, expansions={bfs_stat['expansions']}, time={(t1-t0):.3f}s")
    else:
        print(f"BFS: 无解/超时，expansions={bfs_stat['expansions']}, time={(t1-t0):.3f}s")

    # A*
    t0 = time.perf_counter()
    astar_path, astar_stat = astar_with_stats(PuzzleBoardState(dim=dim, data=init))
    t1 = time.perf_counter()
    if astar_path:
        print(f"A*:  steps={len(astar_path)-1}, expansions={astar_stat['expansions']}, time={(t1-t0):.3f}s")
    else:
        print(f"A*:  无解/超时，expansions={astar_stat['expansions']}, time={(t1-t0):.3f}s")

if __name__ == "__main__":
    # 用例1：简单（2步）
    run_case(
        "Easy (2 moves)",
        3,
        [1,2,3,
         4,0,6,
         7,5,8]
    )


    # 用例3：困难（8-puzzle 最难经典例，最优 31 步）
    # 注意：BFS 在此例会爆炸式增长，可能非常慢或占用巨大内存；A* 能明显更快
    run_case(
        "Hard (31 moves, worst-case)",
        3,
        [8,6,7,
         2,5,4,
         3,0,1]
    )
