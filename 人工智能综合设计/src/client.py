import json
import requests
import getpass
from urllib.parse import urljoin

## TODO: 请同学们实现evaluate函数
from search_engine import evaluate

# 助教在调试完成后会公布会使用的base_url
base_url = 'http://192.144.128.48:34154'  # 本地服务器地址，根据实际情况修改

def input_idx():
    idx = input('idx: ')
    # maybe some restrictions
    return idx

def input_passwd():
    passwd = getpass.getpass('passwd for final submission (None for debug mode): ')
    if passwd == '':
        print('=== DEBUG MODE ===')
    return passwd

def login(idx, passwd):
    url = urljoin(base_url, 'login')
    r = requests.post(url, data={'idx': idx, 'passwd': passwd})
    r_dct = eval(r.text)
    queries = r_dct['queries']
    if r_dct['mode'] == 'illegal':
        raise ValueError('illegal password!')
    elif r_dct['mode'] == 'debug':
        print(f"queries: {queries}")
    print(f'{len(queries)} queries.')
    return queries

def send_ans(idx, passwd, urls):
    url = urljoin(base_url, 'mrr')
    r = requests.post(url, data={'idx': idx, 'passwd': passwd, 'urls': json.dumps(urls)})
    r_dct = eval(r.text)
    if r_dct['mode'] == 'illegal':
        raise ValueError('illegal password!')
    return r_dct['mode'], r_dct['mrr']

def main():
    print("=== 信息检索系统评测客户端 ===")
    print(f"连接到服务器: {base_url}")
    
    idx = input_idx()
    passwd = input_passwd()
    queries = login(idx, passwd)
    print(f"获得 {len(queries)} 个查询")

    print("\n开始处理查询...")
    tot_urls = []

    for index, query in enumerate(queries):
        print(f"处理查询 {index+1}/{len(queries)}")
        urls = evaluate(query)
        print(f"返回 {len(urls)} 个结果")
        tot_urls.append(urls)

    print("\n提交结果到服务器...")
    mode, mrr = send_ans(idx, passwd, tot_urls)
    print(f'MRR@20: [{mrr}], [{mode}] mode')
    
    if mrr == -1:
        print("错误: 您已经提交过测试，每人只能测试一次！")
    elif mrr > 0:
        print(f"测试完成！您的MRR得分: {mrr:.4f}")
    else:
        print("测试出现问题，请检查。")

if __name__ == '__main__':
    main()
