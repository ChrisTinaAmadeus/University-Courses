import copy
import math
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.autograd import Variable
import matplotlib.pyplot as plt
import seaborn as sns

PAD = 0  # padding占位符的索引
UNK = 1  # 未登录词标识符的索引
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")


class Embeddings(nn.Module):
    def __init__(self, d_model, vocab):
        super(Embeddings, self).__init__()
        # Embedding层
        self.lut = nn.Embedding(vocab, d_model)
        # Embedding维数
        self.d_model = d_model

    def forward(self, x):
        # 返回x的词向量（需要乘以math.sqrt(d_model)）
        return self.lut(x) * math.sqrt(self.d_model)


class PositionalEncoding(nn.Module):
    def __init__(self, d_model, dropout, max_len=5000):
        super(PositionalEncoding, self).__init__()
        self.dropout = nn.Dropout(p=dropout)
        # 位置编码矩阵，维度[max_len, embedding_dim]
        pe = torch.zeros(max_len, d_model, device=DEVICE)
        # 单词位置
        position = torch.arange(0.0, max_len, device=DEVICE)
        position.unsqueeze_(1)
        # 使用exp和log实现幂运算
        div_term = torch.exp(
            torch.arange(0.0, d_model, 2, device=DEVICE) * (-math.log(1e4) / d_model)
        )
        div_term.unsqueeze_(0)
        # 计算单词位置沿词向量维度的纹理值
        pe[:, 0::2] = torch.sin(torch.mm(position, div_term))
        pe[:, 1::2] = torch.cos(torch.mm(position, div_term))
        # 增加批次维度，[1, max_len, embedding_dim]
        pe.unsqueeze_(0)
        # 将位置编码矩阵注册为buffer(不参加训练)
        self.register_buffer("pe", pe)

    def forward(self, x):
        # 将一个批次中语句所有词向量与位置编码相加
        # 注意，位置编码不参与训练，因此设置requires_grad=False
        x += Variable(self.pe[:, : x.size(1), :], requires_grad=False)
        return self.dropout(x)


# emb_dim = 64
# max_seq_len = 100
# seq_len = 20

# pe = PositionalEncoding(emb_dim, 0, max_seq_len)
# positional_encoding = pe(torch.zeros(1, seq_len, emb_dim, device=DEVICE))
# plt.figure()
# sns.heatmap(positional_encoding.squeeze().to("cpu"))
# plt.xlabel("i")
# plt.ylabel("pos")
# plt.show()

# plt.figure()
# y = positional_encoding.to("cpu").numpy()
# plt.plot(np.arange(seq_len), y[0, :, 0:64:8], ".")
# plt.legend(["dim %d" % p for p in [0, 7, 15, 31, 63]])
# plt.show()


def clones(module, N):
    """
    克隆基本单元，克隆的单元之间参数不共享
    """
    return nn.ModuleList([copy.deepcopy(module) for _ in range(N)])


def attention(query, key, value, mask=None, dropout=None):
    """
    Scaled Dot-Product Attention（方程（4））
    """
    ### TODO 基础 task 1：实现 Self-Attention 的计算
    scores = torch.matmul(query, key.transpose(-2, -1)) / math.sqrt(query.size(-1))
    # mask
    if mask is not None:
        scores = scores.masked_fill(mask == 0, -1e9)
    # softmax
    p_attn = F.softmax(scores, dim=-1)
    # dropout
    if dropout is not None:
        p_attn = dropout(p_attn)
    # 注意力对v加权
    return torch.matmul(p_attn, value), p_attn


class MultiHeadedAttention(nn.Module):
    """
    Multi-Head Attention（编码器（2））
    """

    def __init__(self, h, d_model, dropout=0.1):
        super(MultiHeadedAttention, self).__init__()
        """
        `h`：注意力头的数量
        `d_model`：词向量维数
        """
        # 确保整除
        assert d_model % h == 0
        # q、k、v向量维数
        self.d_k = d_model // h
        # 头的数量
        self.h = h
        # WQ、WK、WV矩阵及多头注意力拼接变换矩阵WO
        self.linears = clones(nn.Linear(d_model, d_model), 4)
        self.attn = None
        self.dropout = nn.Dropout(p=dropout)

    def forward(self, query, key, value, mask=None):
        if mask is not None:
            mask = mask.unsqueeze(1)
        # 批次大小
        nbatches = query.size(0)
        # WQ、WK、WV分别对词向量线性变换，并将结果拆成h块
        query, key, value = [
            l(x).view(nbatches, -1, self.h, self.d_k).transpose(1, 2)
            for l, x in zip(self.linears, (query, key, value))
        ]
        # 注意力加权
        x, self.attn = attention(query, key, value, mask=mask, dropout=self.dropout)
        # 多头注意力加权拼接
        x = x.transpose(1, 2).contiguous().view(nbatches, -1, self.h * self.d_k)
        # 对多头注意力加权拼接结果线性变换
        return self.linears[-1](x)


class LayerNorm(nn.Module):
    def __init__(self, features, eps=1e-6):
        super(LayerNorm, self).__init__()
        # α、β分别初始化为1、0
        self.a_2 = nn.Parameter(torch.ones(features))
        self.b_2 = nn.Parameter(torch.zeros(features))
        # 平滑项
        self.eps = eps

    def forward(self, x):
        # 沿词向量方向计算均值和方差
        mean = x.mean(dim=-1, keepdim=True)
        std = x.std(dim=-1, keepdim=True)
        # 沿词向量和语句序列方向计算均值和方差
        # mean = x.mean(dim=[-2, -1], keepdim=True)
        # std = x.std(dim=[-2, -1], keepdim=True)
        # 归一化
        x = (x - mean) / torch.sqrt(std**2 + self.eps)
        return self.a_2 * x + self.b_2


class SublayerConnection(nn.Module):
    """
    通过层归一化和残差连接，连接Multi-Head Attention和Feed Forward
    """

    def __init__(self, size, dropout):
        super(SublayerConnection, self).__init__()
        self.norm = LayerNorm(size)
        self.dropout = nn.Dropout(dropout)

    def forward(self, x, sublayer):
        # 层归一化
        x_ = self.norm(x)
        x_ = sublayer(x_)
        x_ = self.dropout(x_)
        # 残差连接
        return x + x_


class PositionwiseFeedForward(nn.Module):
    def __init__(self, d_model, d_ff, dropout=0.1):
        super(PositionwiseFeedForward, self).__init__()
        self.w_1 = nn.Linear(d_model, d_ff)
        self.w_2 = nn.Linear(d_ff, d_model)
        self.dropout = nn.Dropout(dropout)

    def forward(self, x):
        x = self.w_1(x)
        x = F.relu(x)
        x = self.dropout(x)
        x = self.w_2(x)
        return x


# emb_dim = 64
# max_seq_len = 100
# seq_len = 20

# pe = PositionalEncoding(emb_dim, 0, max_seq_len)
# positional_encoding = pe(torch.zeros(1, seq_len, emb_dim, device=DEVICE))
# plt.figure()
# sns.heatmap(positional_encoding.squeeze().to("cpu"))
# plt.xlabel("i")
# plt.ylabel("pos")
# plt.show()

# plt.figure()
# y = positional_encoding.to("cpu").numpy()
# plt.plot(np.arange(seq_len), y[0, :, 0 : 64 : 8], ".")
# plt.legend(["dim %d" % p for p in [0, 7, 15, 31, 63]])
# plt.show()


class EncoderLayer(nn.Module):
    def __init__(self, size, self_attn, feed_forward, dropout):
        super(EncoderLayer, self).__init__()
        self.self_attn = self_attn
        self.feed_forward = feed_forward
        # SublayerConnection作用连接multi和ffn
        self.sublayer = clones(SublayerConnection(size, dropout), 2)
        # d_model
        self.size = size

    def forward(self, x, mask):
        # 将embedding层进行Multi head Attention
        x = self.sublayer[0](x, lambda x: self.self_attn(x, x, x, mask))
        # attn的结果直接作为下一层输入
        return self.sublayer[1](x, self.feed_forward)


class Encoder(nn.Module):
    def __init__(self, layer, N):
        """
        layer = EncoderLayer
        """
        super(Encoder, self).__init__()
        # 复制N个编码器基本单元
        self.layers = clones(layer, N)
        # 层归一化
        self.norm = LayerNorm(layer.size)
        # 新增: 用于可视化保存每一层的注意力权重 (list of tensors)
        self.attn_maps = []

    def forward(self, x, mask):
        """
        循环编码器基本单元N次
        """
        # 每次encode调用前清空保存结构
        self.attn_maps = []
        for layer in self.layers:
            x = layer(x, mask)
            # 保存当前层的 self-attn 权重 (shape: batch, heads, seq_len, seq_len)
            # 注意: MultiHeadedAttention 在 forward 后会将最近一次注意力矩阵保存在 .attn
            if getattr(layer.self_attn, "attn", None) is not None:
                self.attn_maps.append(layer.self_attn.attn.detach())
        return self.norm(x)


class Decoder(nn.Module):
    def __init__(self, layer, N):
        super(Decoder, self).__init__()
        self.layers = clones(layer, N)
        self.norm = LayerNorm(layer.size)

    def forward(self, x, memory, src_mask, tgt_mask):
        """
        循环解码器基本单元N次
        """
        for layer in self.layers:
            x = layer(x, memory, src_mask, tgt_mask)
        return self.norm(x)


class DecoderLayer(nn.Module):
    def __init__(self, size, self_attn, src_attn, feed_forward, dropout):
        super(DecoderLayer, self).__init__()
        self.size = size
        # 自注意力机制
        self.self_attn = self_attn
        # 上下文注意力机制
        self.src_attn = src_attn
        self.feed_forward = feed_forward
        self.sublayer = clones(SublayerConnection(size, dropout), 3)

    def forward(self, x, memory, src_mask, tgt_mask):
        # memory为编码器输出隐表示
        m = memory
        # 自注意力机制，q、k、v均来自解码器隐表示
        x = self.sublayer[0](x, lambda x: self.self_attn(x, x, x, tgt_mask))
        # 上下文注意力机制：q为来自解码器隐表示，而k、v为编码器隐表示
        # BUGFIX: 原实现误用 self.self_attn 进行跨编码器-解码器注意力，正确应使用 self.src_attn
        x = self.sublayer[1](x, lambda x: self.src_attn(x, m, m, src_mask))
        return self.sublayer[2](x, self.feed_forward)


class Generator(nn.Module):
    """
    解码器输出经线性变换和softmax函数映射为下一时刻预测单词的概率分布
    """

    def __init__(self, d_model, vocab):
        super(Generator, self).__init__()
        # decode后的结果，先进入一个全连接层变为词典大小的向量
        self.proj = nn.Linear(d_model, vocab)

    def forward(self, x):
        # 然后再进行log_softmax操作(在softmax结果上再做多一次log运算)
        return F.log_softmax(self.proj(x), dim=-1)


def subsequent_mask(size):
    "Mask out subsequent positions."
    # 设定subsequent_mask矩阵的shape
    attn_shape = (1, size, size)
    # 生成一个右上角(不含主对角线)为全1，左下角(含主对角线)为全0的subsequent_mask矩阵
    subsequent_mask = np.triu(np.ones(attn_shape), k=1).astype("uint8")
    # 返回一个右上角(不含主对角线)为全False，左下角(含主对角线)为全True的subsequent_mask矩阵
    return torch.from_numpy(subsequent_mask) == 0


# plt.figure(figsize=(5, 5))
# plt.imshow(subsequent_mask(20)[0])
# plt.show()


class Batch:
    """
    批次类
        1. 输入序列（源）
        2. 输出序列（目标）
        3. 构造掩码
    """

    def __init__(self, src, trg=None, pad=PAD):
        # 将输入、输出单词id表示的数据规范成整数类型
        src = torch.from_numpy(src).to(DEVICE).long()
        trg = torch.from_numpy(trg).to(DEVICE).long()
        self.src = src
        # 对于当前输入的语句非空部分进行判断，bool序列
        # 并在seq length前面增加一维，形成维度为 1×seq length 的矩阵
        self.src_mask = (src != pad).unsqueeze(-2)
        # 如果输出目标不为空，则需要对解码器使用的目标语句进行掩码
        if trg is not None:
            # TODO 拓展 question 1 : 为什么trg舍弃最后一个字符，trg_y舍弃第一个字符？
            # 解码器使用的目标输入部分
            self.trg = trg[:, :-1]  # 去除最后一列
            # 解码器训练时应预测输出的目标结果
            self.trg_y = trg[:, 1:]  # 去除第一列的SOS
            # 将目标输入部分进行注意力掩码
            self.trg_mask = self.make_std_mask(self.trg, pad)
            # 将应输出的目标结果中实际的词数进行统计
            self.ntokens = (self.trg_y != pad).data.sum()

    # 掩码操作
    @staticmethod
    def make_std_mask(tgt, pad):
        "Create a mask to hide padding and future words."
        tgt_mask = (tgt != pad).unsqueeze(-2)
        tgt_mask = tgt_mask & Variable(
            subsequent_mask(tgt.size(-1)).type_as(tgt_mask.data)
        )
        return tgt_mask


class Transformer(nn.Module):
    def __init__(self, encoder, decoder, src_embed, tgt_embed, generator):
        super(Transformer, self).__init__()
        self.encoder = encoder
        self.decoder = decoder
        self.src_embed = src_embed
        self.tgt_embed = tgt_embed
        self.generator = generator

    def encode(self, src, src_mask):
        return self.encoder(self.src_embed(src), src_mask)

    def decode(self, memory, src_mask, tgt, tgt_mask):
        return self.decoder(self.tgt_embed(tgt), memory, src_mask, tgt_mask)

    def forward(self, src, tgt, src_mask, tgt_mask):
        # encoder的结果作为decoder的memory参数传入，进行decode
        # TODO 基础 - task 2: 调用 encode/decode 实现 Transformer 的 forward
        memory = self.encode(src, src_mask)
        return self.decode(memory, src_mask, tgt, tgt_mask)


def make_model(src_vocab, tgt_vocab, N=6, d_model=512, d_ff=2048, h=8, dropout=0.1):
    c = copy.deepcopy
    # 实例化Attention对象
    attn = MultiHeadedAttention(h, d_model).to(DEVICE)
    # 实例化FeedForward对象
    ff = PositionwiseFeedForward(d_model, d_ff, dropout).to(DEVICE)
    # 实例化PositionalEncoding对象
    position = PositionalEncoding(d_model, dropout).to(DEVICE)
    # 实例化Transformer模型对象
    model = Transformer(
        Encoder(EncoderLayer(d_model, c(attn), c(ff), dropout).to(DEVICE), N).to(
            DEVICE
        ),
        Decoder(
            DecoderLayer(d_model, c(attn), c(attn), c(ff), dropout).to(DEVICE), N
        ).to(DEVICE),
        nn.Sequential(Embeddings(d_model, src_vocab).to(DEVICE), c(position)),
        nn.Sequential(Embeddings(d_model, tgt_vocab).to(DEVICE), c(position)),
        Generator(d_model, tgt_vocab),
    ).to(DEVICE)

    # This was important from their code.
    # Initialize parameters with Glorot / fan_avg.
    for p in model.parameters():
        if p.dim() > 1:
            # 这里初始化采用的是nn.init.xavier_uniform
            nn.init.xavier_uniform_(p)
    return model.to(DEVICE)


class LabelSmoothing(nn.Module):
    """
    标签平滑
    """

    def __init__(self, size, padding_idx, smoothing=0.0):
        super(LabelSmoothing, self).__init__()
        self.criterion = nn.KLDivLoss(reduction="sum")
        self.padding_idx = padding_idx
        self.confidence = 1.0 - smoothing
        self.smoothing = smoothing
        self.size = size
        self.true_dist = None

    def forward(self, x, target):
        assert x.size(1) == self.size
        true_dist = x.data.clone()
        true_dist.fill_(self.smoothing / (self.size - 2))
        true_dist.scatter_(1, target.data.unsqueeze(1), self.confidence)
        true_dist[:, self.padding_idx] = 0
        mask = torch.nonzero(target.data == self.padding_idx)
        if mask.dim() > 0:
            true_dist.index_fill_(0, mask.squeeze(), 0.0)
        self.true_dist = true_dist
        return self.criterion(x, Variable(true_dist, requires_grad=False))


# # Label smoothing的例子
# crit = LabelSmoothing(5, 0, 0.4)  # 设定一个ϵ=0.4
# predict = torch.FloatTensor([[0, 0.2, 0.7, 0.1, 0],
#                              [0, 0.2, 0.7, 0.1, 0],
#                              [0, 0.2, 0.7, 0.1, 0]])
# v = crit(Variable(predict.log()),
#          Variable(torch.LongTensor([2, 1, 0])))

# # Show the target distributions expected by the system.
# print(crit.true_dist)
# plt.imshow(crit.true_dist)


class SimpleLossCompute:
    """
    简单的计算损失和进行参数反向传播更新训练的函数
    """

    def __init__(self, generator, criterion, opt=None):
        self.generator = generator
        self.criterion = criterion
        self.opt = opt

    def __call__(self, x, y, norm):
        x = self.generator(x)
        loss = (
            self.criterion(x.contiguous().view(-1, x.size(-1)), y.contiguous().view(-1))
            / norm
        )
        loss.backward()
        if self.opt is not None:
            self.opt.step()
            self.opt.optimizer.zero_grad()
        return loss.data.item() * norm.float()
