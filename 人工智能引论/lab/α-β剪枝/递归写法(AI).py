# coding:utf-8
"""
Lab 2
井字棋(Tic tac toe)Python语言实现, 带有Alpha-Beta剪枝的Minimax算法.
"""
import random

# 棋盘位置表示（0-8）:
# 0  1  2
# 3  4  5
# 6  7  8

# 设定获胜的组合方式(横、竖、斜)
WINNING_TRIADS = (
    (0, 1, 2),
    (3, 4, 5),
    (6, 7, 8),
    (0, 3, 6),
    (1, 4, 7),
    (2, 5, 8),
    (0, 4, 8),
    (2, 4, 6),
)
# 设定棋盘按一行三个打印
PRINTING_TRIADS = ((0, 1, 2), (3, 4, 5), (6, 7, 8))
# 用一维列表表示棋盘:
SLOTS = (0, 1, 2, 3, 4, 5, 6, 7, 8)
# -1表示X玩家 0表示空位 1表示O玩家.
X_token = -1
Open_token = 0
O_token = 1

MARKERS = ["_", "O", "X"]
END_PHRASE = ("平局", "胜利", "失败")


def print_board(board):
    """打印当前棋盘"""
    for row in PRINTING_TRIADS:
        r = " "
        for hole in row:
            r += MARKERS[board[hole]] + " "
        print(r)


def legal_move_left(board):
    """判断棋盘上是否还有空位"""
    for slot in SLOTS:
        if board[slot] == Open_token:
            return True
    return False


def winner(board):
    """判断局面的胜者,返回值-1表示X获胜,1表示O获胜,0表示平局或者未结束"""
    for triad in WINNING_TRIADS:
        triad_sum = board[triad[0]] + board[triad[1]] + board[triad[2]]
        if triad_sum == 3 or triad_sum == -3:
            return board[triad[0]]  # 表示棋子的数值恰好也是-1:X,1:O
    return 0


def determine_move(board):
    """
    决定电脑(玩家O)的下一步棋(使用Alpha-beta 剪枝优化搜索效率)
    Args:
        board (list):井字棋盘

    Returns:
        next_move(int): 电脑(玩家O) 下一步棋的位置

    """

    # 评估函数: 终局直接用 winner() 的返回值 (1: 电脑O赢, -1: 玩家X赢, 0: 未分胜负或平局)
    def terminal_value(bd):
        w = winner(bd)
        if w != 0:  # 有赢家
            return w
        if not legal_move_left(bd):  # 平局
            return 0
        return None  # 非终局

    # Alpha-Beta 递归搜索: is_maximizing 表示轮到 O (电脑) 时为 True, 轮到 X (玩家) 为 False
    def alpha_beta(bd, is_maximizing, alpha, beta):
        tv = terminal_value(bd)
        if tv is not None:
            return tv

        if is_maximizing:  # 电脑O回合，极大层
            value = float("-inf")
            for pos in SLOTS:
                if bd[pos] == Open_token:
                    bd[pos] = O_token
                    score = alpha_beta(bd, False, alpha, beta)
                    bd[pos] = Open_token
                    if score > value:
                        value = score
                    if value > alpha:
                        alpha = value
                    if alpha >= beta:  # Beta 剪枝
                        break
            return value
        else:  # 玩家X回合，极小层
            value = float("inf")
            for pos in SLOTS:
                if bd[pos] == Open_token:
                    bd[pos] = X_token
                    score = alpha_beta(bd, True, alpha, beta)
                    bd[pos] = Open_token
                    if score < value:
                        value = score
                    if value < beta:
                        beta = value
                    if alpha >= beta:  # Alpha 剪枝
                        break
            return value

    best_score = float("-inf")
    best_moves = []

    # 遍历所有可落子点，模拟电脑O下子并搜索
    for pos in SLOTS:
        if board[pos] == Open_token:
            board[pos] = O_token
            score = alpha_beta(board, False, float("-inf"), float("inf"))
            board[pos] = Open_token
            if score > best_score:
                best_score = score
                best_moves = [pos]
            elif score == best_score:
                best_moves.append(pos)

    # 安全兜底: 如果未找到(理论上不会)，随机一个空位
    if not best_moves:
        for pos in SLOTS:
            if board[pos] == Open_token:
                return pos
        return 0  # 棋盘已满，不会走到这里

    # 多个同等最优点时随机化，避免完全僵硬
    return random.choice(best_moves)


HUMAN = 1
COMPUTER = 0


def main():
    """主函数,先决定谁是X(先手方),再开始下棋"""
    next_move = HUMAN
    opt = input("请选择先手方，输入X表示玩家先手，输入O表示电脑先手：").strip().upper()
    if opt == "X":
        next_move = HUMAN
    elif opt == "O":
        next_move = COMPUTER
    else:
        print("输入有误，默认玩家先手")

    # 初始化空棋盘
    board = [Open_token for i in range(9)]

    # 开始下棋
    while legal_move_left(board) and winner(board) == Open_token:
        print()
        print_board(board)
        if next_move == HUMAN and legal_move_left(board):
            try:
                print("\n")
                humanmv = int(input("请输入你要落子的位置(0-8)："))
                if board[humanmv] != Open_token:
                    continue
                board[humanmv] = X_token
                next_move = COMPUTER
            except:
                print("输入有误，请重试")
                continue
        if next_move == COMPUTER and legal_move_left(board):
            mymv = determine_move(board)
            print("Computer最终决定下在", mymv)
            board[mymv] = O_token
            next_move = HUMAN

    # 输出结果
    print_board(board)
    print(["平局", "Computer赢了", "你赢了"][winner(board)])


if __name__ == "__main__":
    main()
