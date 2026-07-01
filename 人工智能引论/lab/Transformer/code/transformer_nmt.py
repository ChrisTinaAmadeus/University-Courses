import copy
import math
import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm
import os
import seaborn as sns
import time
import torch
import torch.nn as nn
import torch.nn.functional as F
import nltk  # 新增: 用于确保分词所需的资源已下载
from nltk.translate.bleu_score import corpus_bleu, SmoothingFunction
import argparse

from collections import Counter
from langconv import Converter
from transformer_model import *
from nltk import word_tokenize
from torch.autograd import Variable


PAD = 0  # padding占位符的索引
UNK = 1  # 未登录词标识符的索引
BATCH_SIZE = 128  # 批次大小
EPOCHS = 10  # 训练轮数
LAYERS = 4  # transformer中encoder、decoder层数
H_NUM = 8  # 多头注意力个数
D_MODEL = 256  # 输入、输出词向量维数
D_FF = 1024  # feed forward全连接层维数
DROPOUT = 0.1  # dropout比例
MAX_LENGTH = 60  # 语句最大长度

TRAIN_FILE = "nmt/en-cn/train.txt"  # 训练集
DEV_FILE = "nmt/en-cn/dev.txt"  # 验证集
SAVE_FILE = "save/model.pt"  # 模型保存路径

# ========= 训练可视化记录结构 =========
TRAIN_LOSS_HISTORY = []  # 每个 epoch 的训练平均 loss
DEV_LOSS_HISTORY = []  # 每个 epoch 的验证平均 loss
LR_HISTORY = []  # 每一个 step 的学习率 (NoamOpt)


def ensure_nltk_tokenizer():
    """确保 NLTK 的 punkt/punkt_tab 分句与分词模型已安装。
    在不同版本 NLTK 中可能需要 'punkt' 与 'punkt_tab' 两个资源。
    """
    try:
        word_tokenize("Test.")
    except LookupError:
        try:
            nltk.download("punkt")
        except Exception:
            pass
        # 新版本可能需要 punkt_tab
        try:
            nltk.download("punkt_tab")
        except Exception:
            pass


def seq_padding(X, padding=PAD):
    """
    按批次（batch）对数据填充、长度对齐
    """
    # 计算该批次各条样本语句长度
    L = [len(x) for x in X]
    # 获取该批次样本中语句长度最大值
    ML = max(L)
    # 遍历该批次样本，如果语句长度小于最大长度，则用padding填充
    return np.array(
        [
            np.concatenate([x, [padding] * (ML - len(x))]) if len(x) < ML else x
            for x in X
        ]
    )


def cht_to_chs(sent):
    sent = Converter("zh-hans").convert(sent)
    sent.encode("utf-8")
    return sent


class PrepareData:
    def __init__(self, train_file, dev_file):
        # 读取数据、分词
        self.train_en, self.train_cn = self.load_data(train_file)
        self.dev_en, self.dev_cn = self.load_data(dev_file)
        # 构建词表
        self.en_word_dict, self.en_total_words, self.en_index_dict = self.build_dict(
            self.train_en
        )
        self.cn_word_dict, self.cn_total_words, self.cn_index_dict = self.build_dict(
            self.train_cn
        )
        # 单词映射为索引
        self.train_en, self.train_cn = self.word2id(
            self.train_en, self.train_cn, self.en_word_dict, self.cn_word_dict
        )
        self.dev_en, self.dev_cn = self.word2id(
            self.dev_en, self.dev_cn, self.en_word_dict, self.cn_word_dict
        )
        # 划分批次、填充、掩码
        self.train_data = self.split_batch(self.train_en, self.train_cn, BATCH_SIZE)
        self.dev_data = self.split_batch(self.dev_en, self.dev_cn, BATCH_SIZE)

    def load_data(self, path):
        """
        读取英文、中文数据
        对每条样本分词并构建包含起始符和终止符的单词列表
        形式如：en = [['BOS', 'i', 'love', 'you', 'EOS'], ['BOS', 'me', 'too', 'EOS'], ...]
                cn = [['BOS', '我', '爱', '你', 'EOS'], ['BOS', '我', '也', '是', 'EOS'], ...]
        """
        en = []
        cn = []
        with open(path, mode="r", encoding="utf-8") as f:
            for line in f.readlines():
                sent_en, sent_cn = line.strip().split("\t")
                sent_en = sent_en.lower()
                sent_cn = cht_to_chs(sent_cn)
                sent_en = ["BOS"] + word_tokenize(sent_en) + ["EOS"]
                # 中文按字符切分
                sent_cn = ["BOS"] + [char for char in sent_cn] + ["EOS"]
                en.append(sent_en)
                cn.append(sent_cn)
        return en, cn

    def build_dict(self, sentences, max_words=5e4):
        """
        构造分词后的列表数据
        构建单词-索引映射（key为单词，value为id值）
        """
        # 统计数据集中单词词频
        word_count = Counter([word for sent in sentences for word in sent])
        # 按词频保留前max_words个单词构建词典
        # 添加UNK和PAD两个单词
        ls = word_count.most_common(int(max_words))
        total_words = len(ls) + 2
        word_dict = {w[0]: index + 2 for index, w in enumerate(ls)}
        word_dict["UNK"] = UNK
        word_dict["PAD"] = PAD
        # 构建id2word映射
        index_dict = {v: k for k, v in word_dict.items()}
        return word_dict, total_words, index_dict

    def word2id(self, en, cn, en_dict, cn_dict, sort=True):
        """
        将英文、中文单词列表转为单词索引列表
        `sort=True`表示以英文语句长度排序，以便按批次填充时，同批次语句填充尽量少
        """
        length = len(en)
        # 单词映射为索引
        out_en_ids = [[en_dict.get(word, UNK) for word in sent] for sent in en]
        out_cn_ids = [[cn_dict.get(word, UNK) for word in sent] for sent in cn]

        # 按照语句长度排序
        def len_argsort(seq):
            """
            传入一系列语句数据(分好词的列表形式)，
            按照语句长度排序后，返回排序后原来各语句在数据中的索引下标
            """
            return sorted(range(len(seq)), key=lambda x: len(seq[x]))

        # 按相同顺序对中文、英文样本排序
        if sort:
            # 以英文语句长度排序
            sorted_index = len_argsort(out_en_ids)
            out_en_ids = [out_en_ids[idx] for idx in sorted_index]
            out_cn_ids = [out_cn_ids[idx] for idx in sorted_index]
        return out_en_ids, out_cn_ids

    def split_batch(self, en, cn, batch_size, shuffle=True):
        """
        划分批次
        `shuffle=True`表示对各批次顺序随机打乱
        """
        # 每隔batch_size取一个索引作为后续batch的起始索引
        idx_list = np.arange(0, len(en), batch_size)
        # 起始索引随机打乱
        if shuffle:
            np.random.shuffle(idx_list)
        # 存放所有批次的语句索引
        batch_indexs = []
        for idx in idx_list:
            """
            形如[array([4, 5, 6, 7]),
                 array([0, 1, 2, 3]),
                 array([8, 9, 10, 11]),
                 ...]
            """
            # 起始索引最大的批次可能发生越界，要限定其索引
            batch_indexs.append(np.arange(idx, min(idx + batch_size, len(en))))
        # 构建批次列表
        batches = []
        for batch_index in batch_indexs:
            # 按当前批次的样本索引采样
            batch_en = [en[index] for index in batch_index]
            batch_cn = [cn[index] for index in batch_index]
            # 对当前批次中所有语句填充、对齐长度
            # 维度为：batch_size * 当前批次中语句的最大长度
            batch_cn = seq_padding(batch_cn)
            batch_en = seq_padding(batch_en)
            # 将当前批次添加到批次列表
            # Batch类用于实现注意力掩码
            batches.append(Batch(batch_en, batch_cn))
        return batches


class NoamOpt:
    "Optim wrapper that implements rate."

    def __init__(self, model_size, factor, warmup, optimizer):
        self.optimizer = optimizer
        self._step = 0
        self.warmup = warmup
        self.factor = factor
        self.model_size = model_size
        self._rate = 0

    def step(self):
        "Update parameters and rate"
        self._step += 1
        rate = self.rate()
        for p in self.optimizer.param_groups:
            p["lr"] = rate
        self._rate = rate
        self.optimizer.step()

    def rate(self, step=None):
        "Implement `lrate` above"
        if step is None:
            step = self._step
        return self.factor * (
            self.model_size ** (-0.5)
            * min(step ** (-0.5), step * self.warmup ** (-1.5))
        )


def get_std_opt(model):
    return NoamOpt(
        model.src_embed[0].d_model,
        2,
        4000,
        torch.optim.Adam(model.parameters(), lr=0, betas=(0.9, 0.98), eps=1e-9),
    )


def run_epoch(data, model, loss_compute, epoch):
    start = time.time()
    total_tokens = 0.0  # 累计 token 数 (Python float)
    total_loss = 0.0  # 累计 loss (Python float)
    tokens = 0.0  # 当前统计的 token 数 (打印用)

    step = 0
    progress_bar = tqdm(data, ncols=100)
    for batch in progress_bar:
        out = model(batch.src, batch.trg, batch.src_mask, batch.trg_mask)
        loss = loss_compute(out, batch.trg_y, batch.ntokens)  # 返回为张量乘以张量
        loss_scalar = loss.item()  # 将当前批次 loss 转为 Python float
        ntokens_scalar = (
            batch.ntokens.item() if torch.is_tensor(batch.ntokens) else batch.ntokens
        )

        total_loss += loss_scalar
        total_tokens += ntokens_scalar
        tokens += ntokens_scalar

        progress_bar.set_description_str(
            f"Epoch {epoch} Batch: {step} Loss: {loss_scalar / ntokens_scalar:.4f}"
        )
        step += 1
        # 记录当前学习率 (仅训练阶段有 optimizer)
        if getattr(loss_compute, "opt", None) is not None:
            LR_HISTORY.append(loss_compute.opt._rate)

    return total_loss / total_tokens


def train(data, model, criterion, optimizer):
    """
    训练并保存模型
    """
    # 初始化模型在dev集上的最优Loss为一个较大值
    best_dev_loss = 1e5

    for epoch in range(EPOCHS):
        # 模型训练
        model.train()
        train_loss = run_epoch(
            data.train_data,
            model,
            SimpleLossCompute(model.generator, criterion, optimizer),
            epoch,
        )
        TRAIN_LOSS_HISTORY.append(train_loss)
        model.eval()

        # 在dev集上进行loss评估
        print(">>>>> Evaluating on dev set...")
        dev_loss = run_epoch(
            data.dev_data,
            model,
            SimpleLossCompute(model.generator, criterion, None),
            epoch,
        )
        print("<<<<< Evaluate loss: %f" % dev_loss)
        DEV_LOSS_HISTORY.append(dev_loss)

        # 如果当前epoch的模型在dev集上的loss优于之前记录的最优loss则保存当前模型，并更新最优loss值
        if dev_loss < best_dev_loss:
            torch.save(model.state_dict(), SAVE_FILE)
            best_dev_loss = dev_loss
            print("****** Save model done! ******")

        # 追加: 每个 epoch 结束打印汇总（便于日志解析与报告更新）
        print(
            f"[EPOCH_SUMMARY] epoch={epoch+1} train_loss={train_loss:.6f} dev_loss={dev_loss:.6f}"
        )

        print()

    # 训练结束后绘制曲线并注意力可视化
    plot_training_curves()
    try:
        generate_attention_figures(model, data)
    except Exception as e:
        print(f"[WARN] 注意力图生成失败: {e}")


def plot_training_curves():
    """绘制并保存训练与验证 Loss 曲线，以及学习率调度曲线"""
    os.makedirs(os.path.dirname(SAVE_FILE), exist_ok=True)
    # Loss 曲线
    if TRAIN_LOSS_HISTORY and DEV_LOSS_HISTORY:
        plt.figure(figsize=(7, 4))
        plt.plot(
            range(1, len(TRAIN_LOSS_HISTORY) + 1),
            TRAIN_LOSS_HISTORY,
            label="Train Loss",
        )
        plt.plot(
            range(1, len(DEV_LOSS_HISTORY) + 1), DEV_LOSS_HISTORY, label="Dev Loss"
        )
        plt.xlabel("Epoch")
        plt.ylabel("Loss")
        plt.title("Train vs Dev Loss")
        plt.legend()
        plt.tight_layout()
        fig_path = os.path.join(os.path.dirname(SAVE_FILE), "train_dev_loss.png")
        plt.savefig(fig_path)
        plt.close()
        print(f"[FIG] 保存损失曲线 -> {fig_path}")
    else:
        print("[FIG] Loss 历史为空, 未生成损失曲线")

    # 学习率调度曲线
    if LR_HISTORY:
        plt.figure(figsize=(7, 4))
        plt.plot(LR_HISTORY, label="Learning Rate")
        plt.xlabel("Step")
        plt.ylabel("LR")
        plt.title("Noam Learning Rate Schedule")
        plt.legend()
        plt.tight_layout()
        lr_path = os.path.join(os.path.dirname(SAVE_FILE), "learning_rate.png")
        plt.savefig(lr_path)
        plt.close()
        print(f"[FIG] 保存学习率曲线 -> {lr_path}")
    else:
        print("[FIG] 学习率历史为空, 未生成学习率曲线")


def generate_attention_figures(model, data, num_layers=2, num_heads=4):
    """生成前若干层的多头自注意力热力图 (Encoder Self-Attention)"""
    # 选取 dev 的第一批，如果不存在用 train 的第一批
    batch = data.dev_data[0] if data.dev_data else data.train_data[0]
    src = batch.src
    src_mask = batch.src_mask
    _ = model.encode(src, src_mask)  # 调用后 encoder.attn_maps 被填充
    attn_maps = getattr(model.encoder, "attn_maps", [])
    if not attn_maps:
        print("[ATTN] 未捕获到注意力权重, 跳过图像生成")
        return
    os.makedirs(os.path.dirname(SAVE_FILE), exist_ok=True)
    token_ids = src[0].tolist()
    # 去除末尾 PAD
    if PAD in token_ids:
        try:
            pad_pos = token_ids.index(PAD)
            token_ids = token_ids[:pad_pos]
        except ValueError:
            pass
    tokens = [data.en_index_dict.get(i, "UNK") for i in token_ids]
    max_layers = min(num_layers, len(attn_maps))
    for l in range(max_layers):
        layer_attn = attn_maps[l][0]  # (heads, seq_len, seq_len)
        heads = layer_attn.size(0)
        for h in range(min(num_heads, heads)):
            plt.figure(figsize=(6, 5))
            sns.heatmap(
                layer_attn[h].detach().cpu().numpy(),
                xticklabels=tokens,
                yticklabels=tokens,
                cmap="viridis",
            )
            plt.xticks(rotation=45, ha="right")
            plt.yticks(rotation=0)
            plt.title(f"Encoder Layer {l+1} Head {h+1} Attention")
            plt.tight_layout()
            attn_path = os.path.join(
                os.path.dirname(SAVE_FILE), f"attn_layer{l+1}_head{h+1}.png"
            )
            plt.savefig(attn_path)
            plt.close()
            print(f"[FIG] 保存注意力图 -> {attn_path}")


def do_train():
    # 数据预处理
    # 确保分词资源可用
    ensure_nltk_tokenizer()
    data = PrepareData(TRAIN_FILE, DEV_FILE)
    src_vocab = len(data.en_word_dict)
    tgt_vocab = len(data.cn_word_dict)
    print("src_vocab %d" % src_vocab)
    print("tgt_vocab %d" % tgt_vocab)

    # 初始化模型
    model = make_model(src_vocab, tgt_vocab, LAYERS, D_MODEL, D_FF, H_NUM, DROPOUT)

    # 训练
    print(">>>>>>> Training starts!\n")
    train_start = time.time()
    criterion = LabelSmoothing(tgt_vocab, padding_idx=0, smoothing=0.0)
    optimizer = NoamOpt(
        D_MODEL,
        1,
        500,
        torch.optim.Adam(model.parameters(), lr=0, betas=(0.9, 0.98), eps=1e-9),
    )

    train(data, model, criterion, optimizer)
    print(f"<<<<<<< Training finished. Cost {time.time()-train_start:.4f} seconds.")


def greedy_decode(model, src, src_mask, max_len, start_symbol):
    """
    传入一个训练好的模型，对指定数据进行预测
    """
    # TODO 拓展 - question 2: 训练过程和测试过程decoder的输入有何不同？
    # 先用encoder进行encode
    memory = model.encode(src, src_mask)
    ys = torch.ones(1, 1).fill_(start_symbol).type_as(src.data)
    # 遍历输出的长度下标
    for i in range(max_len - 1):
        # decode得到隐层表示
        out = model.decode(
            memory,
            src_mask,
            Variable(ys),
            Variable(subsequent_mask(ys.size(1)).type_as(src.data)),
        )
        # 将隐藏表示转为对词典各词的log_softmax概率分布表示
        prob = model.generator(out[:, -1])
        # 获取当前位置最大概率的预测词id
        _, next_word = torch.max(prob, dim=1)
        next_word = next_word.data[0]
        # 将当前位置预测的字符id与之前的预测内容拼接起来
        ys = torch.cat([ys, torch.ones(1, 1).type_as(src.data).fill_(next_word)], dim=1)
    return ys


def beam_search_decode(
    model, src, src_mask, max_len, start_symbol, beam_size=5, length_penalty=0.7
):
    """简单 Beam Search 解码 (不含重复惩罚)。
    length_penalty < 1.0 会稍微偏向长序列，>1.0 偏向短序列，这里用 <1 让 EOS 不至于太早。
    返回最佳候选的 token 序列 (tensor: 1 x L)。
    """
    model.eval()
    with torch.no_grad():
        memory = model.encode(src, src_mask)
        # 每个 beam: (tokens_tensor, log_prob)
        beams = [
            (torch.tensor([[start_symbol]], dtype=src.dtype, device=src.device), 0.0)
        ]
        finished = []
        for step in range(1, max_len):
            new_beams = []
            for tokens, logp in beams:
                if tokens[0, -1].item() == start_symbol and tokens.size(1) > 1:
                    # 已经生成 EOS (假设 start_symbol 用作 BOS, EOS 区分? 此处不提前结束, 具体根据你的词表调整)
                    finished.append((tokens, logp))
                    continue
                tgt_mask = subsequent_mask(tokens.size(1)).type_as(src.data)
                out = model.decode(memory, src_mask, tokens, tgt_mask)
                prob = model.generator(out[:, -1])  # (vocab,)
                log_probs = torch.log_softmax(prob, dim=-1).squeeze(0)  # vocab
                topk_logp, topk_idx = torch.topk(log_probs, beam_size)
                for k_logp, k_idx in zip(topk_logp.tolist(), topk_idx.tolist()):
                    new_seq = torch.cat(
                        [tokens, torch.tensor([[k_idx]], device=src.device)], dim=1
                    )
                    # length penalty 应用于总 logp / len^lp
                    new_beams.append((new_seq, logp + k_logp))
            # 选取得分最高的 beam_size 个，使用长度惩罚归一化排序
            if not new_beams:
                break
            new_beams = sorted(
                new_beams,
                key=lambda x: x[1] / (x[0].size(1) ** length_penalty),
                reverse=True,
            )[:beam_size]
            beams = new_beams
        # 汇总候选
        all_candidates = finished + beams
        if not all_candidates:
            return torch.tensor([[start_symbol]], device=src.device)
        best = sorted(
            all_candidates,
            key=lambda x: x[1] / (x[0].size(1) ** length_penalty),
            reverse=True,
        )[0][0]
        return best


def batch_translate(model, data, infile, outfile, beam_size=None):
    """批量翻译英文文件 (每行一个英文句子)。"""
    model.eval()
    src_vocab = data.en_word_dict
    tgt_vocab = data.cn_word_dict
    idx2tgt = data.cn_index_dict
    bos_id = tgt_vocab["BOS"]
    eos_token = "EOS"
    results = []
    with open(infile, "r", encoding="utf-8") as fin:
        for line in fin:
            sent = line.strip().lower()
            tokens = ["BOS"] + word_tokenize(sent) + ["EOS"]
            ids = [src_vocab.get(w, UNK) for w in tokens]
            src_tensor = torch.from_numpy(np.array(ids)).long().unsqueeze(0).to(DEVICE)
            src_mask = (src_tensor != PAD).unsqueeze(-2)
            if beam_size and beam_size > 1:
                out = beam_search_decode(
                    model, src_tensor, src_mask, MAX_LENGTH, bos_id, beam_size=beam_size
                )
            else:
                ys = torch.ones(1, 1).fill_(bos_id).type_as(src_tensor.data)
                for _ in range(MAX_LENGTH - 1):
                    out_dec = model.decode(
                        model.encode(src_tensor, src_mask),
                        src_mask,
                        ys,
                        subsequent_mask(ys.size(1)).type_as(src_tensor.data),
                    )
                    prob = model.generator(out_dec[:, -1])
                    _, next_word = torch.max(prob, dim=-1)
                    next_word = next_word.item()
                    ys = torch.cat(
                        [
                            ys,
                            torch.ones(1, 1).type_as(src_tensor.data).fill_(next_word),
                        ],
                        dim=1,
                    )
                out = ys[0]
            # 转中文 token
            trg_tokens = []
            for tid in out.tolist()[1:]:  # skip BOS
                sym = idx2tgt.get(tid, "UNK")
                if sym == eos_token:
                    break
                trg_tokens.append(sym)
            results.append("".join(trg_tokens))
    with open(outfile, "w", encoding="utf-8") as fout:
        for r in results:
            fout.write(r + "\n")
    print(f"[TRANS] 写入翻译结果 {len(results)} 行 -> {outfile}")


def eval_bleu(model, data, sample_size=200, beam_size=None):
    """在 dev 集随机抽样计算 BLEU (粗略)。"""
    from random import sample

    refs = []
    hyps = []
    bos_id = data.cn_word_dict["BOS"]
    eos_token = "EOS"
    indices = sample(range(len(data.dev_en)), min(sample_size, len(data.dev_en)))
    model.eval()
    with torch.no_grad():
        for idx in indices:
            en_tokens = data.dev_en[idx]
            cn_tokens = data.dev_cn[idx]
            en_ids = [data.en_word_dict.get(w, UNK) for w in en_tokens]
            src_tensor = (
                torch.from_numpy(np.array(en_ids)).long().unsqueeze(0).to(DEVICE)
            )
            src_mask = (src_tensor != PAD).unsqueeze(-2)
            if beam_size and beam_size > 1:
                out = beam_search_decode(
                    model, src_tensor, src_mask, MAX_LENGTH, bos_id, beam_size=beam_size
                )
            else:
                out = greedy_decode(model, src_tensor, src_mask, MAX_LENGTH, bos_id)[0]
            # 参考去除 BOS/EOS
            ref = [t for t in cn_tokens[1:-1]]
            hyp = []
            for tid in out.tolist()[1:]:
                sym = data.cn_index_dict.get(tid, "UNK")
                if sym == eos_token:
                    break
                hyp.append(sym)
            refs.append([ref])
            hyps.append(hyp)
    smoothie = SmoothingFunction().method3
    bleu = corpus_bleu(refs, hyps, smoothing_function=smoothie)
    print(f"[BLEU] dev 抽样 {len(hyps)} 条 BLEU={bleu*100:.2f}")
    return bleu


def translate():
    # 初始化模型
    data = PrepareData(TRAIN_FILE, DEV_FILE)
    src_vocab = len(data.en_word_dict)
    tgt_vocab = len(data.cn_word_dict)
    # print("src_vocab %d" % src_vocab)
    # print("tgt_vocab %d" % tgt_vocab)

    model = make_model(src_vocab, tgt_vocab, LAYERS, D_MODEL, D_FF, H_NUM, DROPOUT)
    # 加载模型
    model.load_state_dict(torch.load(SAVE_FILE))
    with torch.no_grad():
        while True:
            # 输入待翻译的英文语句
            sentence_en = input("请输入英文: ")
            # 打印待翻译的英文语句
            sentence_en = sentence_en.lower()
            # TODO 基础 - task 3: 将输入的英文语句分词并转为单词 id 表示
            sentence_en = ["BOS"] + word_tokenize(sentence_en) + ["EOS"]
            sentence_en_ids = [data.en_word_dict.get(w, UNK) for w in sentence_en]

            en_sent = " ".join([data.en_index_dict[w] for w in sentence_en_ids])
            # print("\n" + en_sent)

            # 将当前以单词id表示的英文语句数据转为tensor，并放如DEVICE中
            src = torch.from_numpy(np.array(sentence_en_ids)).long().to(DEVICE)
            # 增加一维
            src = src.unsqueeze(0)
            # 设置attention mask
            src_mask = (src != 0).unsqueeze(-2)
            # 用训练好的模型进行decode预测
            out = greedy_decode(
                model,
                src,
                src_mask,
                max_len=MAX_LENGTH,
                start_symbol=data.cn_word_dict["BOS"],
            )
            # 初始化一个用于存放模型翻译结果语句单词的列表
            translation = []
            # 遍历翻译输出字符的下标（注意：开始符"BOS"的索引0不遍历）
            for j in range(1, out.size(1)):
                # 获取当前下标的输出字符
                sym = data.cn_index_dict[out[0, j].item()]
                # 如果输出字符不为'EOS'终止符，则添加到当前语句的翻译结果列表
                if sym != "EOS":
                    translation.append(sym)
                # 否则终止遍历
                else:
                    break
            # 打印模型翻译输出的中文语句结果
            print("译文: %s\n" % " ".join(translation))


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Transformer NMT Usage")
    parser.add_argument(
        "--mode",
        choices=["train", "translate", "batch", "bleu"],
        default="train",
        help="运行模式",
    )
    parser.add_argument(
        "--beam", type=int, default=0, help="Beam size (>1 启用 beam search)"
    )
    parser.add_argument(
        "--input", type=str, default=None, help="batch 翻译输入英文文件路径"
    )
    parser.add_argument(
        "--output", type=str, default=None, help="batch 翻译输出中文文件路径"
    )
    parser.add_argument("--sample", type=int, default=200, help="BLEU 抽样条数")
    args = parser.parse_args()

    ensure_nltk_tokenizer()
    data = PrepareData(TRAIN_FILE, DEV_FILE)
    src_vocab = len(data.en_word_dict)
    tgt_vocab = len(data.cn_word_dict)
    model = make_model(src_vocab, tgt_vocab, LAYERS, D_MODEL, D_FF, H_NUM, DROPOUT)

    if args.mode == "train":
        criterion = LabelSmoothing(tgt_vocab, padding_idx=0, smoothing=0.0)
        optimizer = NoamOpt(
            D_MODEL,
            1,
            500,
            torch.optim.Adam(model.parameters(), lr=0, betas=(0.9, 0.98), eps=1e-9),
        )
        train(data, model, criterion, optimizer)
    else:
        # 加载已训练模型
        if os.path.exists(SAVE_FILE):
            model.load_state_dict(torch.load(SAVE_FILE, map_location=DEVICE))
            print(f"[LOAD] 已加载模型: {SAVE_FILE}")
        else:
            raise FileNotFoundError(f"未找到已训练模型 {SAVE_FILE}, 请先 --mode train")

        if args.mode == "translate":
            bos_id = data.cn_word_dict["BOS"]
            while True:
                try:
                    sent = input("英文句子(回车结束, Ctrl+C 退出): ").strip()
                except KeyboardInterrupt:
                    break
                if not sent:
                    continue
                tokens = ["BOS"] + word_tokenize(sent.lower()) + ["EOS"]
                ids = [data.en_word_dict.get(w, UNK) for w in tokens]
                src_tensor = (
                    torch.from_numpy(np.array(ids)).long().unsqueeze(0).to(DEVICE)
                )
                src_mask = (src_tensor != PAD).unsqueeze(-2)
                if args.beam and args.beam > 1:
                    out = beam_search_decode(
                        model,
                        src_tensor,
                        src_mask,
                        MAX_LENGTH,
                        bos_id,
                        beam_size=args.beam,
                    )[0]
                else:
                    out = greedy_decode(
                        model, src_tensor, src_mask, MAX_LENGTH, bos_id
                    )[0]
                translation = []
                for tid in out.tolist()[1:]:
                    sym = data.cn_index_dict.get(tid, "UNK")
                    if sym == "EOS":
                        break
                    translation.append(sym)
                print("译文:", "".join(translation))
        elif args.mode == "batch":
            if not args.input or not args.output:
                raise ValueError("batch 模式需要 --input 和 --output")
            beam_size = args.beam if args.beam > 1 else None
            batch_translate(model, data, args.input, args.output, beam_size=beam_size)
        elif args.mode == "bleu":
            beam_size = args.beam if args.beam > 1 else None
            eval_bleu(model, data, sample_size=args.sample, beam_size=beam_size)
