#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <limits>
#include <unordered_map>
#define LIST_SIZE 100

// 统一返回码
#define OK 1
#define ERROR -1
#define OVERFLOW -2

typedef int Status;
typedef string Elemtype;
typedef struct
{
    Elemtype *elem; // 动态数组首地址
    int length;     // 当前词数
    int listsize;   // 已分配容量（最大可容纳的词数）
} SqList;

// 建立顺序表，初始化所有元素
Status InitList_Sq(SqList &L)
{
    L.elem = new Elemtype[LIST_SIZE];
    if (!L.elem)
        return OVERFLOW; // 异常处理
    L.length = 0;
    L.listsize = LIST_SIZE;
    return OK;
}

// 扩容顺序表，每次增加LIST_INIT_SIZE的长度
Status Expand_Capacity(SqList &L)
{
    int newSize = L.listsize + LIST_SIZE;
    Elemtype *newBase = new (nothrow) Elemtype[newSize];
    if (!newBase)
        return OVERFLOW; // 异常处理
    for (int i = 0; i < L.length; ++i)
        newBase[i] = move(L.elem[i]);
    delete[] L.elem;
    L.elem = newBase;
    L.listsize = newSize;
    return OK;
}

// 将一个词复制并追加到顺序表
Status Append_Word(SqList &L, const char *begin, int len)
{
    if (len == 0)
        return OK;
    if (L.length + 1 > L.listsize)
        if (Expand_Capacity(L) != OK)
            return OVERFLOW; // 随时进行扩容
    L.elem[L.length++] = string(begin, len);
    return OK;
}

// 销毁顺序表，释放内存
void Destroy_List(SqList &L)
{
    if (!L.elem)
        return; // 异常处理
    delete[] L.elem;
    L.elem = nullptr;
    L.length = 0;
    L.listsize = 0;
}

// 判断当前位置是否为空格或标点，并记录其大小用于后续分词
bool is_Space_Or_Punct(string &s, int i, int &delimLen)
{
    delimLen = 0;
    if (i >= s.size())
        return false;

    unsigned char c = (unsigned char)s[i];
    // ASCII 空白或标点（单字节）
    if (isspace(c) || ispunct(c))
    {
        delimLen = 1;
        return true;
    }

    // 常见中文（全角）标点（多字节）
    static const vector<string> CN_PUNCTS = {
        "，", "。", "！", "？", "；", "：", "、",
        "（", "）", "《", "》", "【", "】",
        "“", "”", "‘", "’", "——", "……"};

    for (const auto &p : CN_PUNCTS)
    {
        if (i + p.size() <= s.size() && s.compare(i, p.size(), p) == 0)
        {
            delimLen = p.size();
            return true;
        }
    }
    return false;
}

// 分词，遇到空格或标点时切断，将词存入顺序表。
Status String_cut(string &input, SqList &L)
{
    int i = 0, n = input.size();
    int word_start = 0;
    int word_len = 0;

    // 从头遍历找单词
    while (i < n)
    {
        int dlen = 0;
        if (is_Space_Or_Punct(input, i, dlen))
        {
            if (word_len > 0)
            {
                Append_Word(L, input.data() + word_start, word_len);
                word_len = 0;
            }
            i += dlen; // 跳过整个分隔符（支持多字节中文标点）
        }
        else // 词未结束
        {
            if (word_len == 0)
                word_start = i;
            ++i;
            ++word_len;
        }
    }

    // 使结尾的单词被添加
    if (word_len > 0)
    {
        Append_Word(L, input.data() + word_start, word_len);
    }

    return OK;
}

// 打印字符串中的所有词
void Print_String(SqList &L, int print_type)
{
    // 设置两种打印模式用于反映中英文文本的实际展示情况
    cout << "-------------------------------------------" << endl;
    if (print_type)
        for (int i = 0; i < L.length; i++)
            cout << L.elem[i];
    else
        for (int i = 0; i < L.length; i++)
            cout << L.elem[i] << " ";
    cout << endl
         << "-------------------------------------------" << endl;
}

// 插入一个或多个单词
Status Word_Insert(SqList &L, int i, SqList &Ins)
{
    cout << "-------------------------------------------" << endl;

    // 确保i的值位于正常区间
    if (i < 1)
        i = 1;
    if (i > L.length + 1)
        i = L.length + 1;
    if (Ins.length <= 0)
        return OK;

    // 确保容量
    while (L.length + Ins.length > L.listsize)
    {
        if (Expand_Capacity(L) != OK)
            return OVERFLOW;
    }

    // 从尾到头右移（包含 i-1 位置），为插入腾出空间
    for (int p = L.length - 1; p >= i - 1; --p)
        L.elem[p + Ins.length] = L.elem[p];

    // 复制 Ins.length 个新词到 [i-1, i-1+Ins.length)
    for (int j = 0; j < Ins.length; ++j)
        L.elem[i - 1 + j] = Ins.elem[j];

    L.length += Ins.length;
    cout << "新字符串为：";
    for (int i = 0; i < L.length; i++)
        cout << L.elem[i] << " ";
    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 删除一个或多个单词
Status Word_Delete(SqList &L, int i, int num)
{
    cout << "-------------------------------------------" << endl;

    if (i < 1 || i > L.length || i + num - 1 > L.length)
        return ERROR; // 异常处理
    if (num <= 0)
        return OK;
    for (int p = i - 1; p < L.length - num; p++)
    {
        L.elem[p] = L.elem[p + num];
    }
    L.length -= num;

    cout << "新字符串为：";
    for (int i = 0; i < L.length; i++)
        cout << L.elem[i] << " ";
    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 倒置字符串
Status String_Reverse(SqList &L)
{
    cout << "-------------------------------------------" << endl;
    Elemtype tmp;

    // 首尾交换实现倒置
    for (int i = 0; i < (L.length / 2); i++)
    {
        tmp = L.elem[i];
        L.elem[i] = L.elem[L.length - i - 1];
        L.elem[L.length - i - 1] = tmp;
    }

    cout << "新字符串为：";
    for (int i = 0; i < L.length; i++)
        cout << L.elem[i] << " ";
    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 判断字符串是否为回文串
Status Judge_Palindrome(SqList &L)
{
    cout << "-------------------------------------------" << endl;

    for (int i = 0; i < (L.length / 2); i++)
    {
        // 首尾比较进行判断
        if (L.elem[i] != L.elem[L.length - i - 1])
        {
            cout << "该字符串不是回文串！";
            cout << endl
                 << "-------------------------------------------" << endl
                 << endl;
            return OK;
        }
    }
    cout << "该字符串是回文串！";
    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 计算单词个数
Status String_Length(SqList &L)
{
    cout << "-------------------------------------------" << endl;

    cout << "字符串共有" << L.length << "个单词！";

    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 查找一个字符串
Status Find_Word(SqList &L, SqList &t)
{
    cout << "-------------------------------------------" << endl;
    if (t.length == 0)
    {
        cout << "查找为空，请重新输入！";
        cout << endl
             << "-------------------------------------------" << endl
             << endl;
        return OK;
    }
    // 直接遍历进行查找
    if (t.length == 1)
    {
        for (int i = 0; i < L.length; i++)
        {
            if (L.elem[i] == t.elem[0])
            {
                cout << "在第" << i + 1 << "个词找到了“" << t.elem[0] << "”!";
                cout << endl
                     << "-------------------------------------------" << endl
                     << endl;
                return OK;
            }
        }
        cout << "抱歉！“" << t.elem[0] << "”不存在！";
    }
    else
    {
        for (int i = 0; i < L.length; i++)
        {
            if (L.elem[i] == t.elem[0])
            {
                bool find_string = true;
                for (int j = 1; j < t.length; j++)
                {
                    if (L.elem[i + j] != t.elem[j])
                    {
                        find_string = false;
                        break;
                    }
                }
                if (find_string)
                {
                    cout << "在第" << i + 1 << "个词至第" << i + t.length << "个词找到了目标字符串！";
                    cout << endl
                         << "-------------------------------------------" << endl
                         << endl;
                    return OK;
                }
            }
        }
        cout<<"抱歉！目标字符串不存在！";
    }
    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 词的统计
Status Count_Word(SqList &L)
{
    cout << "-------------------------------------------" << endl;
    if (L.length == 0)
    {
        cout << "当前字符串为空，无法统计！";
        cout << endl
             << "-------------------------------------------" << endl
             << endl;
        return OK;
    }
    int *flat = new int[L.listsize];
    fill(flat, flat + L.listsize, 0);
    int all_count = 0;      // 已计入的单词总数
    int diff_count = 0;     // 不同词数量
    int word_count = 0;     // 当前词计数
    Elemtype tmp;           // 当前统计的词
    bool count_all = false; // 判断是否统计完毕

    for (int i = 0; !count_all; i++)
    {
        // 如果当前位置已统计过
        if (flat[i])
        {
            // 如果已到本轮末尾，需要完成一次收尾并回到起点
            if (i == L.length - 1)
            {
                if (!tmp.empty())
                {
                    diff_count++;
                    cout << tmp << ":" << word_count << "次" << endl;
                    tmp.clear();
                    word_count = 0;
                }
                i = -1; // for 的 i++ 会使其变成 0
            }
            continue;
        }

        // 进入新词统计
        if (tmp.empty())
            tmp = L.elem[i];
        if (tmp == L.elem[i])
        {
            word_count++;
            flat[i] = 1;
            all_count++;
            if (all_count == L.length)
            {
                i = L.length - 1;
                count_all = true;
            }
        }

        // 到达末尾时，输出当前词的统计并回到起点
        if (i == L.length - 1)
        {
            diff_count++;
            cout << tmp << ":" << word_count << "次" << endl;
            tmp.clear();
            word_count = 0;
            i = -1; // for 的 i++ 会使其变成 0
        }
    }
    cout << "字符串中一共有" << diff_count << "个不同的词";
    delete[] flat; // 即时释放flat内存

    cout << endl
         << "-------------------------------------------" << endl
         << endl;
    return OK;
}

// 获取字符串
Status Load_SqList(SqList *L)
{
    cout << "您好！欢迎使用顺序表处理字符串\n";
    cout << "请选择您的输入类型(0代表控制台,1代表文件):";
    int mode;
    cin >> mode;
    // 清理上一条整数输入遗留在缓冲区中的换行，避免后续 getline 直接读到空行
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string input;
    if (mode)
    {
        cout << "请输入文件路径: ";
        string path;
        getline(cin, path);
        ifstream fin(path, ios::in | ios::binary);
        if (!fin)
        {
            cerr << "无法打开文件: " << path << endl;
            return ERROR;
        }
        input.assign((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
        fin.close();
    }
    else
    {
        cout << "请输入待处理的字符串: ";
        getline(cin, input);
    }

    if (InitList_Sq(*L) != OK)
    {
        cerr << "初始化顺序表失败，请重试！\n";
        return ERROR;
    }

    if (String_cut(input, *L) != OK)
    {
        cerr << "分词失败\n";
        Destroy_List(*L);
        return ERROR;
    }
    return OK;
}

// 主程序
// 读入文本 → 执行操作 → 释放内存
int main()
{
    SqList L_list;
    SqList *L = &L_list;
    if (Load_SqList(L) != OK)
    {
        cerr << "读取字符串失败！请重试~" << endl;
        return ERROR;
    }
    int method = 0;
    bool stop = false;
    while (!stop)
    {
        cout << "请选择您想进行的操作（打印其序号）：" << endl
             << "1  打印当前字符串" << endl
             << "2  插入1或多个单词" << endl
             << "3  删除1或多个单词" << endl
             << "4  倒置字符串" << endl
             << "5  判断字符串是否为回文串" << endl
             << "6  计算单词个数" << endl
             << "7  查找单词/字符串是否存在" << endl
             << "8  统计每个词出现多少次" << endl
             << "9  输入新的字符串" << endl
             << "10  销毁当前字符串，并结束操作" << endl
             << "您选择的操作是：";

        // 精确读取命令，避免陷入死循环
        //  读取整行以避免非数字导致cin进入失败状态
        string method_line;
        if (!getline(cin, method_line))
        {
            cerr << "读取输入失败，请重试~" << endl;
            break; // 读不到输入直接退出主循环
        }
        // 去掉前后空白
        method_line.erase(0, method_line.find_first_not_of(" \t\r\n"));
        method_line.erase(method_line.find_last_not_of(" \t\r\n") + 1);
        try
        {
            size_t idx = 0;
            long long val = stoll(method_line, &idx, 10);
            if (idx != method_line.size())
            {
                throw invalid_argument("extra characters");
            }
            method = static_cast<int>(val);
        }
        catch (const exception &)
        {
            cout << "无效输入（请键入 1-10 的数字）。" << endl
                 << "-------------------------------------------" << endl;
            this_thread::sleep_for(chrono::milliseconds(600));
            continue; // 进入下一轮菜单
        }

        bool operation = true;
        switch (method)
        {
        case 1:
        {
            cout << "1表示直接打印,0表示打印时每个词之间有空格" << endl
                 << "请选择打印模式：";
            int print_type = 1;
            cin >> print_type;
            // 丢弃本行剩余的换行，避免影响下一次 getline 读取菜单
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            Print_String(L_list, print_type);
            cout << "字符串打印完毕！" << endl;
            break;
        }
        case 2:
        {
            cout << "请输入您想要插入的位置（从第1个开始）：";
            int position = 1;
            cin >> position;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            cout << "请输入所有即将插入的单词，多于1个词则以空格或标点隔开：";
            string line;
            getline(cin, line);

            // 使用已有分词函数将一整行按空白/标点切成单词
            SqList tmp;
            if (InitList_Sq(tmp) != OK)
            {
                cerr << "初始化临时表失败！请重试~" << endl;
                break;
            }
            if (String_cut(line, tmp) != OK)
            {
                cerr << "分词失败！请重试~" << endl;
                Destroy_List(tmp);
                break;
            }

            if (Word_Insert(L_list, position, tmp) != OK)
            {
                cerr << "插入失败!请重试~" << endl;
                break;
            }
            Destroy_List(tmp);
            break;
        }
        case 3:
        {
            cout << "请输入删除的起始位置（从第一个开始）：";
            int position = 0;
            cin >> position;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "请输入您想要删除的单词个数：";
            int num = 0;
            cin >> num;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (Word_Delete(L_list, position, num) != OK)
            {
                cerr << "删除失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 4:
        {
            if (String_Reverse(L_list) != OK)
            {
                cerr << "倒置字符串失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 5:
        {
            if (Judge_Palindrome(L_list) != OK)
            {
                cerr << "回文串判断失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 6:
        {
            if (String_Length(L_list) != OK)
            {
                cerr << "单词个数获取失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 7:
        {
            cout << "请输入您想要查找的单词/字符串：";
            string line;
            getline(cin, line);
            SqList target;
            if (InitList_Sq(target) != OK)
            {
                cerr << "初始化临时表失败！请重试~" << endl;
                break;
            }
            if (String_cut(line, target) != OK)
            {
                cerr << "分词失败！请重试~" << endl;
                Destroy_List(target);
                break;
            }
            if (Find_Word(L_list, target) != OK)
            {
                cerr << "查找操作失败！请重试~";
                break;
            }
            break;
        }
        case 8:
        {
            if (Count_Word(L_list) != OK)
            {
                cerr << "统计失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 9:
        {
            if (Load_SqList(L) != OK)
            {
                cerr << "获取字符串失败！请重试~" << endl;
                break;
            }
            break;
        }
        case 10:
        {
            Destroy_List(L_list);
            cout << "已成功销毁字符串！" << endl;
            stop = true;
            break;
        }
        default:
        {
            operation = false;
            cout << "******************************************" << endl;
            cout << "八嘎呀路！给我输入正确的操作序号啊混蛋！！！" << endl;
            cout << "******************************************" << endl;
            break;
        }
        }
        if (operation)
            cout << "您选择的操作已执行完毕！" << endl
                 << "[-----------------1秒后继续操作-----------------]" << endl;
        // 小停顿便于阅读连续输出
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
    return 0;
}