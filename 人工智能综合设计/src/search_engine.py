import dill as pickle
import jieba
from collections import Counter, defaultdict
import numpy as np
import os
import re
# 切换工作目录到当前文件所在目录，保证相对路径正确
os.chdir(os.path.dirname(__file__))


# 加载搜索停用词表（用于过滤查询中的无意义词）
with open('query_stopwords.txt', 'r', encoding='utf-8') as f:
    query_stopwords = [line.strip() for line in f]
    

# 加载所有必要的索引和参数（倒排索引、文档长度、URL映射、分词-文档对、文档正文）
with open("./inverted_index.pkl", "rb") as f:
    inverted_index = pickle.load(f)
with open("./doc_length_dict_len.pkl", "rb") as f:
    doc_length_dict = pickle.load(f)
with open("./url_map.pkl", "rb") as f:
    url_map = pickle.load(f)
with open("./term_docid_pairs.pkl", "rb") as f:
    term_docid_pairs = pickle.load(f)
with open('./doc_text_dict.pkl', 'rb') as f:
    doc_text_dict = pickle.load(f)


# 对查询进行分词并去除停用词
def clean_query(query):
    terms = jieba.cut(query)
    terms = [term for term in terms if term not in query_stopwords]
    return ''.join(terms)


# 对分词结果中的核心词进行权重提升（如实体、长词）
def identify_and_boost_keywords(terms):
    boosted_terms = []
    for term in terms:
        # 如果是较长的词（很可能是实体或重要概念），重复它来提升权重
        if len(term) >= 4 and not re.search(r'[0-9]', term):  # 排除纯数字
            boosted_terms.extend([term, term])  # 重复一次，相当于权重×2
        else:
            boosted_terms.append(term)
    return boosted_terms


# 获取某个分词的倒排列表（去除开头的特殊标记）
def get_postings_list(inverted_index, query_term):
    try:
        return inverted_index[query_term][1:]
    except KeyError:
        return []


# 计算所有文档的 BM25 分数，返回得分最高的前 k 个文档
def bm25_scores(inverted_index, doc_length_dict, query, k=20, k1=2, b=1):
    scores = defaultdict(float)
    terms = jieba.cut(query)
    # 过滤停用词后提升核心词权重
    filtered_terms = [term for term in terms if term not in query_stopwords]
    boosted_terms = identify_and_boost_keywords(filtered_terms)
    query_terms = Counter(boosted_terms)  # 使用提升后的词频

    N = len(doc_length_dict)  # 文档总数
    avgdl = np.mean(list(doc_length_dict.values()))  # 平均文档长度
    df_dict = Counter(term for term, docid in term_docid_pairs)  # 每个分词的文档频率
    added_bonus_docids = set()  # 连续字符串加分的文档集合
    added_all_terms_bonus_docids = set()  # 所有分词都出现加分的文档集合
    for q in query_terms:
        postings_list = get_postings_list(inverted_index, q)
        df = df_dict.get(q, 0)
        
        # 计算 BM25 的 idf
        idf = np.log(1 + (N - df + 0.5) / (df + 0.5)) if df > 0 else 0.0
        for posting in postings_list:
            tf = posting.tf  # 词频
            dl = doc_length_dict.get(posting.docid, 0)  # 文档长度
            denom = tf + k1 * (1 - b + b * dl / avgdl)  # 分母
            score = idf * (tf * (k1 + 1)) / denom if denom > 0 else 0  # BM25得分
            # 连续字符串加分逻辑：如果原始query作为字符串出现在正文中，额外加分
            if doc_text_dict is not None:
                text = doc_text_dict.get(posting.docid, "")
            if query in text and posting.docid not in added_bonus_docids:
                score += 8  # 基于精确搜索，因此加分偏多
                added_bonus_docids.add(posting.docid)
            # 所有分词都出现加分逻辑：如果很多分词都在正文中，额外加分
            if posting.docid not in added_all_terms_bonus_docids:
                present_terms = [term for term in query_terms if term in text]
                m = len(present_terms)
                n = len(query_terms)
                bonus = 0
                #设置非线性加分规则，奖励更多匹配分词的网页
                if m == n:
                    bonus = 5
                elif m == n - 1:
                    bonus = 2
                elif m == n - 2:
                    bonus = 1
                # 其余情况不加分
                if bonus > 0:
                    score += bonus
                    added_all_terms_bonus_docids.add(posting.docid)

            scores[posting.docid] += score * query_terms[q]*0.8  # 按词频加权
    results = []
    for docid, score in scores.items():
        results.append((docid, score))
    results.sort(key=lambda x: -x[1])  # 按得分降序排序
    return results[:k]


# 根据查询长度和标点调整 BM25 参数（长句降低词频影响，增强长度归一化）
def get_bm25_params(query):
    terms = jieba.cut(query)
    terms = [term for term in terms if term not in query_stopwords]
    query_str = ''.join(terms)
    # 长度大于10认为是长句
    # 降低k1值，使词频影响变小
    # 增加b值，使文档长度归一化更强，避免长文档因包含更多词而得分过高
    if len(query_str) > 10: #长句词频分布更均匀
        return {'k1': 1, 'b': 0.8}
    else: #短查询词频影响大
        return {'k1': 3, 'b': 0.6}

# 检索接口：根据 BM25 算法返回相关性最高的文档及分数
def retrieval_by_bm25(inverted_index, url_map, query, k=20):
    params = get_bm25_params(query)
    top_scores = bm25_scores(inverted_index, doc_length_dict, query, k=k, k1=params['k1'], b=params['b'])
    results = [
        (url_map[str(docid)], score, docid)
        for docid, score in top_scores
        if str(docid) in url_map
    ]
    return results


# 评估接口：输入查询，返回相关的 URL 列表
def evaluate(query):
    results = retrieval_by_bm25(inverted_index, url_map, query)
    url_list = [url for url, score, docid in results]
    return url_list
