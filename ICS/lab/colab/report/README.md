# 报告模板通用指南

通常我们提供包括但不限于 Markdown 格式的报告模板，以便于大家准确精炼地展示助教想要了解的信息。

如果你不使用我们提供的模板，你也需要参考我们提供的模板内容来组织你的报告。

无论我们提供何种格式的报告模板，或你自己选择的制作格式，你都需要在提交时同时提交**源文件**和**转换出的 PDF** ，以便于助教查阅。

> 比如 .tex 和 .pdf 文件。

## 通用报告要求

我们有一些跨 lab 共通的报告要求：

1. 不要贴大段的代码，你可以通过以下手段来避免：
    - 使用伪代码
    - 用注释替代一些不重要的部分
    - 如果确实必要，请一小段一小段地复制，并配合文字解释
2. 如果某段内容很长，请用“总分”的格式来组织，即先分点简述一下，再开始展开。同时，最好先讲你觉得最重要的部分。
3. 不要写大段“助教知道”的内容，除非助教要求，比如解释非你修改/写的代码的内容。
4. 尽量精简，不要用 AI 生成工具的车轱辘话来应付报告，我们宁愿你写得很短。

## Markdown 转 PDF

### 方法一：Typora

https://typora.io/#download

> 我们可能会于上机课上解释 Typora 的用法。但互联网上的资料已经足够丰富。

### 方法二：pandoc

> 这个环境配置占用的空间比较大（>2G），而且转化效果和前者相比，相去甚远，除非迫不得已，推荐使用方法一。

Ubuntu 系统下安装（我们提供的服务器会提前安装好，WSL2 用户可以参考）：

```bash
sudo apt-get install pandoc texlive-xetex texlive-lang-chinese
```

检查系统安装的中文字体：

```bash
fc-list :lang=zh
```

它的输出形如：

```bash
/usr/share/fonts/truetype/arphic/uming.ttc: AR PL UMing TW MBE:style=Light
/usr/share/fonts/truetype/arphic-bkai00mp/bkai00mp.ttf: AR PL KaitiM Big5,文鼎ＰＬ中楷:style=Regular
/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf: Droid Sans Fallback:style=Regular
/usr/share/fonts/truetype/arphic-gkai00mp/gkai00mp.ttf: AR PL KaitiM GB,文鼎ＰＬ简中楷:style=Regular
/usr/share/fonts/truetype/arphic-gbsn00lp/gbsn00lp.ttf: AR PL SungtiL GB,文鼎ＰＬ简报宋:style=Regular
/usr/share/fonts/truetype/arphic/uming.ttc: AR PL UMing TW:style=Light
```

分别对应字体：

- "AR PL UMing TW MBE"
- "AR PL KaitiM Big5" 或 "文鼎ＰＬ中楷"
- "Droid Sans Fallback"

等，即文件名后面到冒号前的部分

转化命令：

```bash
pandoc -s report.md -o report.pdf --pdf-engine=xelatex -V CJKmainfont="Droid Sans Fallback"
```

其中 "Droid Sans Fallback" 可以换成任何你系统中存在的中文字体，但如果报 WANING 支持不全，则需要尝试其他字体。
