from flask import Flask, render_template, request
from search_engine_UI import evaluate
import time

# Flask 应用初始化（模板目录与静态资源采用默认结构：templates/ 与 static/）

app = Flask(__name__)

@app.route('/')
def index():
    """显示搜索首页：仅渲染首页模板"""
    return render_template('index.html')

@app.route('/query')

def query():
    # 内嵌分页页码生成器：根据当前页与总页数返回页码序列（含省略号）
    def get_pagination(page, total_pages):
        pages = []
        if total_pages <= 5:
            # 总页数较少：直接展示所有页码
            pages = list(range(1, total_pages + 1))
        else:
            if page <= 3:
                # 当前页靠前：展示前 5 页 + 末页
                pages = [1, 2, 3, 4, 5, '...', total_pages]
            elif page >= total_pages - 2:
                # 当前页靠后：展示首页 + 最后 5 页
                pages = [1, '...', total_pages - 4, total_pages - 3, total_pages - 2, total_pages - 1, total_pages]
            else:
                # 当前页在中间：展示 首页/末页 + 当前页前后各 1 页
                pages = [1, '...', page - 1, page, page + 1, '...', total_pages]
        return pages

    """处理搜索请求：解析参数 -> 调用检索 -> 分页整理 -> 渲染结果页"""
    # 1) 解析查询参数（key 为关键词，page 为页码，默认第 1 页）
    keyword = request.args.get('key', '')
    page = int(request.args.get('page', 1))

    # 2) 关键词为空则返回首页并提示
    if not keyword:
        return render_template('index.html', error="请输入搜索关键词")
    
    # 3) 调用搜索引擎：记录耗时并捕获异常
    try:
        start_time = time.time()  # 记录开始时间
        results_list = evaluate(keyword)  # 根据关键词检索，返回结果列表
        end_time = time.time()  # 记录结束时间
        cost_time = round(end_time - start_time, 3)  # 计算检索耗时（秒）
    except Exception as e:
        # 检索过程出错：回到首页显示错误信息
        return render_template('index.html', error=f"搜索出错: {str(e)}")
    
    # 4) 结果分页：确定总数/总页数/当前页数据切片
    page_size = 5  # 每页显示条目数
    total_results = len(results_list)  # 总结果数
    total_pages = (total_results + page_size - 1) // page_size  # 向上取整计算总页数
    start = (page - 1) * page_size  # 当前页起始下标
    end = start + page_size  # 当前页结束下标（开区间）
    results = results_list[start:end]  # 当前页数据
    pagination = get_pagination(page, total_pages)  # 生成分页页码

    # 5) 渲染结果页模板
    return render_template(
        'results.html',
        keyword=keyword,            # 原始查询关键词
        results=results,            # 当前页结果列表
        cost_time=cost_time,        # 检索耗时（秒）
        page=page,                  # 当前页码
        total_pages=total_pages,    # 总页数
        total_results=total_results,# 总结果数
        pagination=pagination       # 页码数组（含省略号）
    )

if __name__ == '__main__':
    # 本地启动配置：
    # debug=True    启用调试（代码改动自动重载，便于开发）
    # port=2616     监听端口（需在 0~65535，推荐 >1024）
    # host='0.0.0.0' 绑定所有网卡，允许局域网访问；若仅本机访问可改为 '127.0.0.1'
    app.run(debug=True, port=2616, host='0.0.0.0')