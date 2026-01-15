from urllib.parse import urljoin, urlparse  # 用于URL拼接和解析
from bs4 import BeautifulSoup  # 用于解析HTML文档
import requests  # 用于发送HTTP请求
import time  # 用于设置等待时间
from url_normalize import url_normalize  # 用于标准化URL
import os  # 用于文件和目录操作
import hashlib  # 用于生成内容哈希（未使用）
import threading  # 用于多线程
import queue  # 用于线程安全队列

def crawl_all_urls(html_doc, url):
    """
    从HTML文档中提取所有链接,并标准化为绝对URL。
    :param html_doc: 网页的HTML内容
    :param url: 当前页面的URL(用于相对路径拼接)
    :return: 所有标准化后的URL集合
    """
    all_links = set()
    try:
        soup = BeautifulSoup(html_doc, "html.parser")
    except:
        print("Fail to parse the html document!")
        return all_links
    for anchor in soup.find_all("a"):
        href = anchor.attrs.get("href")
        if href != "" and href != None:
            if not href.startswith("http"):
                # 如果是相对路径，拼接为绝对路径
                href = urljoin(url, href)
            all_links.add(url_normalize(href))
    return all_links


def filter_urls(urls, keywords, blacklist):
    """
    过滤URL,只保留域名在keywords中的且不以黑名单后缀结尾的URL。
    """
    filter_url = [url for url in urls if urlparse(url).netloc in keywords]
    filter_url = [
        url for url in filter_url if not any(url.endswith(black) for black in blacklist)
    ]
    return filter_url


keywords = ["keyan.ruc.edu.cn", "xsc.ruc.edu.cn"]  # 允许爬取的域名
blacklist = [  # 不爬取的文件类型后缀
    ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".svg", ".webp", ".doc",
    ".docx", ".pdf", ".xls", ".xlsx", ".ppt", ".pptx", ".txt", ".rtf", ".mp4", ".avi",
    ".mov", ".wmv", ".flv", ".mkv", ".webm", ".mp3", ".wav", ".aac", ".flac", ".ogg",
    ".m4a", ".zip", ".rar", ".7z", ".tar", ".gz", ".exe", ".msi", ".apk", ".bat",
    ".sh", ".js", ".css", ".iso", ".dmg", ".bin",
]

# 输入种子urls，作为爬虫的起点
input_urls = ["http://keyan.ruc.edu.cn/", "http://xsc.ruc.edu.cn/"]


def get_html(uri, headers={}, timeout=None):
    """
    从指定URL获取HTML文档内容。
    :param uri: 目标URL
    :param headers: 请求头
    :param timeout: 超时时间
    :return: HTML文本或None
    """
    try:
        r = requests.get(uri, headers=headers, timeout=timeout)
        print(f"请求URL: {uri} | HTTP状态码: {r.status_code}")
        r.raise_for_status()
        r.encoding = "UTF-8"
        return r.text
    except:
        return None

# 多线程爬虫相关参数和数据结构
headers = {"user-agent": "my-app/0.0.1"}        # 请求头，伪装为浏览器
url_queue = queue.Queue()                       # 待爬取URL队列
all_urlset = set()                              # 所有已发现的URL集合（防止重复）
used_urlset = set()                              # 已爬取过的URL集合
url_map = {}                                    # 网页编号到URL的映射
content_hash_set = set()                        # 网页内容哈希集合（未使用）
lock = threading.Lock()                         # 线程锁，保证数据安全
wait_time = 1                                   # 每次爬取间隔时间（秒）
max_threads = 24                                 # 最大线程数
output_dir = r"D:\Desktop\编程集训\day2\URL内容(plus)"  # 网页保存目录
os.makedirs(output_dir, exist_ok=True)         # 创建保存目录

# 初始化队列和集合，将种子URL加入队列
for url in input_urls:
    url_queue.put(url)
    all_urlset.add(url)

page_counter = 0  # 网页计数器
def worker():
    """
    爬虫线程工作函数：从队列获取URL，下载网页，提取新URL并保存网页。
    """
    global page_counter
    while True:
        try:
            url = url_queue.get(timeout=10)  # 超时则退出
        except queue.Empty:
            break
        html_doc = get_html(url, headers=headers)
        if html_doc is None:
            url_queue.task_done()
            continue
        with lock:
            used_urlset.add(url)

        # 提取并过滤新URL
        url_sets = crawl_all_urls(html_doc, url)
        url_sets = filter_urls(url_sets, keywords, blacklist)
        with lock:
            for new_url in url_sets:
                if new_url not in all_urlset:
                    url_queue.put(new_url)
                    all_urlset.add(new_url)

        if wait_time > 0:
            print(f"等待{wait_time}秒后开始抓取")
            time.sleep(wait_time)
        # 保存当前网页内容到本地文件
        with lock:
            page_counter += 1
            print(f"已爬取网页总数: {page_counter}", end=' | ')
            print(f"剩余URL数量: {url_queue.qsize()}")
            path = os.path.join(output_dir, f"{page_counter}.html")
            with open(path, "w", encoding="utf-8") as f:
                f.write(html_doc)
            url_map[page_counter] = url
        url_queue.task_done()

# 启动多线程爬虫
threads = []
for _ in range(max_threads):
    t = threading.Thread(target=worker)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

# 保存网页编号与URL的映射关系到txt文件
with open("url_map(plus).txt", "w", encoding="utf-8") as f:
    for idx, url in url_map.items():
        f.write(f"{idx}\t{url}\n")
print("爬取完成！")