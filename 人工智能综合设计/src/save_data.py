import jieba
import os
import string
from collections import defaultdict
import dill as pickle
from bs4 import BeautifulSoup
import re


# 获取所有 html 文件名
collections = [file for file in os.listdir('URL内容(plus)') if os.path.splitext(file)[1] == '.html']
# 按编号从小到大排序
collections.sort(key=lambda x: int(os.path.splitext(x)[0]))

# 读取url_map(plus).txt，建立文件编号到URL的映射
url_map = {}
with open('url_map(plus).txt', encoding='utf-8') as f:
    for line in f:
        parts = line.strip().split('\t')
        if len(parts) == 2:
            idx, url = parts
            url_map[idx] = url

doc_title_dict = {}

for filename in collections:
    docid = int(os.path.splitext(filename)[0])
    with open(os.path.join('URL内容(plus)', filename), encoding='utf-8') as fin:
        html_content = fin.read()
        soup = BeautifulSoup(html_content, 'html.parser')
        # 提取<title>标签内容，如果没有则为空字符串
        title_tag = soup.find('title')
        title = title_tag.get_text(strip=True) if title_tag else ""
        doc_title_dict[docid] = title

# 加载停用词表
with open('百度停用词表.txt', encoding='utf-8') as f:
    stopwords = set(line.strip() for line in f if line.strip()) # 读取停用词并去除空行

# 依次打开每个保存正文信息的文本文件，分词，构造term_docid_pairs,计算文档长度
term_docid_pairs = []
doc_length_dict = {}
doc_text_dict = {}

for filename in collections:
    docid = int(os.path.splitext(filename)[0])  # 直接用文件名数字作为 docid
    with open(os.path.join('URL内容(plus)', filename), encoding='utf-8') as fin:
        html_content = fin.read()
        soup = BeautifulSoup(html_content, 'html.parser')
        # 提取全文（去除标签，仅保留文本）
        full_text = soup.get_text(separator=' ', strip=True)
        match = re.search(r'([^\s。！？]{2,}?[。！？])', full_text)  # 使用正则查找第一个较长的句子作为标题起点
        if match:
            # 以第一个长句为标题起点
            title = match.group(1)
            start_idx = full_text.index(title)
            text = full_text[start_idx:]
        else:
            text = full_text  # 如果没有长句则用全文
        text = re.sub(r'{{.*?}}', '', text) # 去除花括号内容
        text = re.sub(r'\s+', ' ', text)  # 将多个空格替换为一个空格
        if len(text) > 100:
            text = text[:-100] #去掉最后100个字符，过滤无用信息
        doc_text_dict[docid] = text  # 添加到字典
        terms = [] # 存储当前文档的有效分词
        for term in jieba.cut(text):
            term=term.strip()
            # 过滤掉长度小于等于1、空白、标点和停用词
            if (len(term) <= 1 or term.isspace() or term in string.punctuation or term in stopwords):
                continue
            terms.append(term)
            term_docid_pairs.append((term, docid))# 保存分词和文档id

        doc_length_dict[docid] = len(terms)  # 保存文档分词数作为文档长度

# 排序
term_docid_pairs = sorted(term_docid_pairs)

# 构造倒排索引
# postings list中每一项为一个Posting类对象
class Posting(object):
    special_doc_id = -1
    def __init__(self, docid, tf=0):
        self.docid = docid  # 文档id
        self.tf = tf        # 词频
    def __repr__(self):
        return "<docid: %d, tf: %d>" % (self.docid, self.tf)

#在每个postings list开头加上了一个用来标记开头的Posting(special_doc_id=-1, 0)
# （正常docid从0编号）
inverted_index = defaultdict(lambda: [Posting(Posting.special_doc_id, 0)])

for term, docid in term_docid_pairs:
    postings_list = inverted_index[term]
    if docid != postings_list[-1].docid:
        postings_list.append(Posting(docid, 1))
    else:  # 如果相同，说明该词在同一文档出现多次，词频加1
        postings_list[-1].tf += 1

inverted_index = dict(inverted_index)

#保存所有后续需要使用的文件
with open('./inverted_index.pkl', 'wb') as f:
    pickle.dump(inverted_index, f)
with open('./doc_length_dict.pkl', 'wb') as f:
    pickle.dump(doc_length_dict, f)
with open('./url_map.pkl', 'wb') as f:
    pickle.dump(url_map, f)
with open('doc_title_dict.pkl', 'wb') as f:
    pickle.dump(doc_title_dict, f)
with open('./term_docid_pairs.pkl', 'wb') as f:
    pickle.dump(term_docid_pairs, f)
with open('./doc_text_dict.pkl', 'wb') as f:
    pickle.dump(doc_text_dict, f)
