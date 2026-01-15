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
    """打印当前棋盘。

    Args:
        board (list[int]): 长度为 9 的棋盘列表。
    """
    for row in PRINTING_TRIADS:
        r = " "
        for hole in row:
            r += MARKERS[board[hole]] + " "
        print(r)


def legal_move_left(board):
    """判断棋盘上是否还有空位。

    Returns:
        bool: True 表示仍有合法落子点；False 表示棋盘已满。
    """
    for slot in SLOTS:
        if board[slot] == Open_token:
            return True
    return False


def winner(board):
    """判断局面胜者（或尚未结束）。

    Returns:
        int:
            -1  X 获胜
             1  O 获胜
             0  平局或尚未结束（无法仅从和棋区分未结束，因此使用者需结合是否有空位判断）
    """
    for triad in WINNING_TRIADS:
        triad_sum = board[triad[0]] + board[triad[1]] + board[triad[2]]
        if triad_sum == 3 or triad_sum == -3:
            return board[triad[0]]  # 表示棋子的数值恰好也是-1:X,1:O
    return 0

# 找出所有子节点
def find_possible_move(board):
    """列出当前棋盘所有可落子位置。"""
    possible_move = []
    for i in range(len(board)):
        if board[i] == Open_token:
            possible_move.append(i)
    return possible_move


class Node:
    """博弈树节点。

    Attributes:
        move (int|None): 走到当前局面的上一手落子索引；根节点为 None。
        board (list[int]): 当前局面棋盘列表(长度 9)。
        children (list[Node]): 子节点列表。
        value (int|None): 终局评价值；非叶子为 None。这里沿用 winner() 的取值: -1, 0, 1。
    """

    def __init__(self, move=None, board=None, children=None, value=None):
        self.value = value    # 仅叶子（终局）节点有值；内部节点为 None
        self.board = board    # 当前棋盘状态
        self.move = move      # 父节点执行的走法（根节点无）
        self.children = children if children else []

    def get_children(self):
        return self.children

    def evaluate(self):
        return self.value


# 建立节点，将所有子节点添加进去，如果为叶子节点则计算其value
def init_node(move, board, is_human=False):
    """递归构建完整博弈树。

    Args:
        move (int|None): 父节点落子到达此局面的落子索引；根节点为 None。
        board (list[int]): 当前局面。
        is_human (bool): 标记当前轮到谁：
            False -> 轮到电脑 O 落子（我们在最大层）
            True  -> 轮到玩家 X 落子（我们在最小层）

    Returns:
        Node: 构建完成的节点（含子树）。叶子节点的 value 初始值为终局胜负结果；内部节点 value 为 None。
    """
    result = winner(board)
    possible_move = find_possible_move(board)
    # 叶子：有人赢了 或 没有空位
    if result != 0 or not possible_move:
        return Node(move=move, board=board[:], children=[], value=result)

    children = []
    # 轮到谁下取决于 is_human
    token = O_token if not is_human else X_token
    for child_move in possible_move:
        # 为子局面复制棋盘，避免共享引用
        child_board = board[:]
        child_board[child_move] = token
        # 轮换玩家：如果当前是电脑，下完后轮到玩家；反之亦然
        child_node = init_node(child_move, child_board, not is_human)
        children.append(child_node)
    return Node(move=move, board=board[:], children=children, value=None)


# 认为已经构建好了所有节点，从此节点开始遍历子节点，并进行剪枝
def alpha_beta_search(node, alpha, beta, is_max_node=True, depth=0):
    """对已构建的博弈树执行 Alpha-Beta 剪枝搜索，并加入深度敏感评分。

    将叶子局面重新映射：

        - O 赢:  +100 - depth   (越早赢分越高)
        - X 赢:  -100 + depth   (越晚输(被 X 赢)绝对值越小)
        - 平局:  0

    这样在最大化过程中会优先选择最快胜利；在最小化层会拖延己方失败或阻止对手快速胜利。

    Args:
        node (Node): 当前搜索节点。
        alpha (float): 当前已知对 Maximizer (O) 最有利的下界。
        beta (float): 当前已知对 Minimizer (X) 最不利的上界。
        is_max_node (bool): True 表示该层为 O (最大化)；False 表示 X (最小化)。
        depth (int): 当前搜索深度（根为 0）。

    Returns:
        tuple(int|float, int|None): (当前节点最优估值, 在该节点选择的最优落子 move)。

    算法说明：
        - 对最大层：更新 alpha = max(alpha, value)。若 alpha >= beta，后续兄弟剪枝。
        - 对最小层：更新 beta  = min(beta, value)。若 alpha >= beta，后续兄弟剪枝。
    """
    if node.value is not None:  # 叶子节点：根据终局结果返回加权得分
        if node.value == O_token:       # O 赢
            return 100 - depth, node.move
        elif node.value == X_token:     # X 赢
            return -100 + depth, node.move
        else:                           # 平局
            return 0, node.move

    best_move = None
    if is_max_node:  # Maximizer: O
        value = float('-inf')
        for child in node.children:
            child_val, _ = alpha_beta_search(child, alpha, beta, False, depth + 1)
            if child_val > value:
                value = child_val
                best_move = child.move
            alpha = max(alpha, value)
            if alpha >= beta:  # 剪枝
                break
        return value, best_move
    else:  # Minimizer: X
        value = float('inf')
        for child in node.children:
            child_val, _ = alpha_beta_search(child, alpha, beta, True, depth + 1)
            if child_val < value:
                value = child_val
                best_move = child.move
            beta = min(beta, value)
            if alpha >= beta:  # 剪枝
                break
        return value, best_move


def determine_move(board): 
    """决定电脑 (O) 在当前局面的下一步落子位置。

    构建博弈树，然后做Alpha-Beta 搜索。

    Args:
        board (list[int]): 当前棋盘。

    Returns:
        int: 电脑 (O) 下一步落子位置索引。
    """
    node = init_node(None, board, is_human=False)
    _, next_move = alpha_beta_search(node, float('-inf'), float('inf'), True, 0)
    return next_move


HUMAN = 1
COMPUTER = 0


def main():
    """命令行入口：负责交互式人机对弈过程。"""
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
                    # 已被占用，重新输入
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
