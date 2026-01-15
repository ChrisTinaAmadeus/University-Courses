import dill as pickle
import jieba
from collections import Counter, defaultdict
import numpy as np
import os
import re
os.chdir(os.path.dirname(__file__))

# 加载搜索停用词表
with open("./query_stopwords.txt", 'r', encoding='utf-8') as f:
    query_stopwords = [line.strip() for line in f]

# 加载所有必要的索引和参数
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
with open('./doc_title_dict.pkl', 'rb') as f:
    doc_title_dict = pickle.load(f)
# for key,value in doc_title_dict.items():
#     print(f"编号: {key}, 标题: {value}")

def clean_query(query):
    terms = jieba.cut(query)
    terms = [term for term in terms if term not in query_stopwords]
    return ''.join(terms)

def identify_and_boost_keywords(terms):
    boosted_terms = []
    for term in terms:
        # 如果是较长的词（很可能是实体或重要概念），重复它来提升权重
        if len(term) >= 3 and not re.search(r'[0-9]', term):  # 排除纯数字
            boosted_terms.extend([term, term])  # 重复一次，相当于权重×2
        else:
            boosted_terms.append(term)
    return boosted_terms

def get_postings_list(inverted_index, query_term):
    try:
        return inverted_index[query_term][1:]
    except KeyError:
        return []

def bm25_scores(inverted_index, doc_length_dict, query, k=100, k1=2, b=1):
    scores = defaultdict(float)
    terms = jieba.cut(query)
    # 过滤停用词后提升核心词权重
    filtered_terms = [term for term in terms if term not in query_stopwords]
    boosted_terms = identify_and_boost_keywords(filtered_terms)
    
    query_terms = Counter(boosted_terms)  # 使用提升后的词频

    N = len(doc_length_dict)
    avgdl = np.mean(list(doc_length_dict.values()))
    df_dict = Counter(term for term, docid in term_docid_pairs)
    added_bonus_docids = set()
    added_all_terms_bonus_docids = set()
    for q in query_terms:
        postings_list = get_postings_list(inverted_index, q)
        df = df_dict.get(q, 0)
        
        idf = np.log(1 + (N - df + 0.5) / (df + 0.5)) if df > 0 else 0.0
        for posting in postings_list:
            tf = posting.tf
            dl = doc_length_dict.get(posting.docid, 0)
            denom = tf + k1 * (1 - b + b * dl / avgdl)
            score = idf * (tf * (k1 + 1)) / denom if denom > 0 else 0
            # 连续字符串加分逻辑
            if doc_text_dict is not None:
                text = doc_text_dict.get(posting.docid, "")
            if query in text and posting.docid not in added_bonus_docids:
                score += 8  # 或 score *= 1.2
                added_bonus_docids.add(posting.docid)
            # 所有分词都出现加分逻辑
            if posting.docid not in added_all_terms_bonus_docids:
                present_terms = [term for term in query_terms if term in text]
                m = len(present_terms)
                n = len(query_terms)
                bonus = 0
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

            scores[posting.docid] += score * query_terms[q]*0.5
    results = []
    for docid, score in scores.items():
        results.append((docid, score))
    results.sort(key=lambda x: -x[1])
    return results[:k]

def get_bm25_params(query):
    terms = jieba.cut(query)
    terms = [term for term in terms if term not in query_stopwords]
    query_str = ''.join(terms)
    # 长度大于10或包含问号、逗号、句号等认为是长句
    # 降低k1值，使词频影响变小，增加b值，使文档长度归一化更强
    if len(query_str) > 10 or re.search(r'[?,，。.!！？，]', query_str):
        return {'k1': 1, 'b': 0.8}
    else:
        return {'k1': 4, 'b': 0.5}
    
def retrieval_by_bm25(inverted_index, url_map, query, k=100):
    params = get_bm25_params(query)
    top_scores = bm25_scores(inverted_index, doc_length_dict, query, k=k, k1=params['k1'], b=params['b'])
    results = [
        (url_map[str(docid)], score, docid)
        for docid, score in top_scores
        if str(docid) in url_map
    ]
    return results


def evaluate(query):
    
    results = retrieval_by_bm25(inverted_index, url_map, query)
    result_list = []
    for url, score, docid in results:
        title = doc_title_dict.get(docid, "无标题")
        snippet = doc_text_dict.get(docid, "")[:100]
        result_list.append({
            "title": title,
            "url": url,
            "snippet": snippet,
            "score": score
        })
    
    return result_list
