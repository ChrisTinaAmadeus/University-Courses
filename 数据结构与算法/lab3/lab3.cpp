#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;
#define OK 1
#define ERROR 0
#define MAX_SIZE 100
typedef int Status;

// 树
enum NodeType
{
    ELEMENT_NODE, // 元素结点
    TEXT_NODE     // 文本结点
};
struct Node
{
    NodeType type;             // 结点类型
    char *tag_name;            // 标签名：div, p, a等 (仅元素结点)
    char *id;                  // id属性
    char **classes;            // class列表 (字符串数组)
    int class_count;           // class数量
    char *href;                // href属性 (仅a标签)
    char *text;                // 文本内容 (仅文本结点)
    struct Node *parent;       // 父结点指针
    struct Node *children;     // 第一个子结点指针 (左孩子)
    struct Node *next_sibling; // 下一个兄弟结点指针 (右兄弟)
};
Status InitNode(Node *&node, NodeType type)
{
    node->type = type;
    node->tag_name = nullptr;
    node->id = nullptr;
    node->classes = nullptr;
    node->class_count = 0;
    node->href = nullptr;
    node->text = nullptr;
    node->parent = nullptr;
    node->children = nullptr;
    node->next_sibling = nullptr;
    return OK;
}
void FreeTree(Node *node)
{
    if (!node)
        return;
    // 递归释放子结点
    FreeTree(node->children);
    // 递归释放兄弟结点
    FreeTree(node->next_sibling);
    // 释放当前结点资源
    if (node->type == ELEMENT_NODE)
    {
        if (node->tag_name)
            free(node->tag_name);
        if (node->id)
            free(node->id);
        if (node->classes)
        {
            for (int i = 0; i < node->class_count; ++i)
            {
                if (node->classes[i])
                    free(node->classes[i]);
            }
            free(node->classes);
        }
        if (node->href)
            free(node->href);
    }
    else if (node->type == TEXT_NODE)
    {
        if (node->text)
            free(node->text);
    }
    free(node);
}

// 栈
struct Stack
{
    Node **base;   // 栈底指针
    Node **top;    // 栈顶指针
    int stacksize; // 栈容量
};
Status InitStack(Stack &s)
{
    s.base = (Node **)malloc(MAX_SIZE * sizeof(Node *));
    if (!s.base)
    {
        return ERROR; // 内存分配失败
    }
    s.top = s.base;
    s.stacksize = MAX_SIZE;
    return OK;
}
Status Push(Stack &s, Node *item)
{
    if (s.top - s.base >= s.stacksize)
    {
        // 扩容
        s.base = (Node **)realloc(s.base, (s.stacksize + MAX_SIZE) * sizeof(Node *));
        if (!s.base)
        {
            return ERROR; // 内存分配失败
        }
        s.top = s.base + s.stacksize;
        s.stacksize += MAX_SIZE;
    }
    *s.top++ = item;
    return OK;
}
Status Pop(Stack &s, Node *&item)
{
    if (s.top == s.base)
    {
        return ERROR; // 栈空
    }
    item = *--s.top;
    return OK;
}
Status GetTop(Stack &s, Node *&item)
{
    if (s.top == s.base)
    {
        return ERROR; // 栈空
    }
    item = *(s.top - 1);
    return OK;
}
Status StackEmpty(Stack &s)
{
    return s.top == s.base ? OK : ERROR;
}

// 查询结果列表
struct NodeList
{
    struct Node **nodes;
    int count;
    int capacity;
};

// 加载HTML文件
Status load_html(const char *path, Node *&root)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        return ERROR;
    }
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配内存
    char *html_content = (char *)malloc(length + 1);
    if (!html_content)
    {
        fclose(file);
        return ERROR;
    }
    // 读取内容
    fread(html_content, 1, length, file);
    html_content[length] = '\0'; // 确保字符串以空字符结尾
    fclose(file);
    // 初始化栈
    Stack stack;
    InitStack(stack);
    // 创建根结点
    root = (Node *)malloc(sizeof(Node));
    InitNode(root, ELEMENT_NODE);
    root->tag_name = strdup("root");
    Push(stack, root);
    // 构建整棵DOM树
    for (int i = 0; i < length; ++i)
    {
        if (html_content[i] == '<') // 处理标签
        {
            if (html_content[i + 1] == '/') // 处理结束标签 </...
            {
                int j = i + 2;
                while (html_content[j] != '>' && j < length)
                    j++;
                Node *pop_item;
                Pop(stack, pop_item);
                i = j; // 出栈并更新索引
            }
            else if (html_content[i + 1] == '!') // 处理注释 <!-- ... --> 或 DOCTYPE <!DOCTYPE ...>
            {
                if (html_content[i + 2] == '-' && html_content[i + 3] == '-')
                {
                    // 注释 <!-- ... -->
                    int j = i + 4;
                    while (j < length - 2)
                    {
                        if (html_content[j] == '-' && html_content[j + 1] == '-' && html_content[j + 2] == '>')
                        {
                            j += 2;
                            break;
                        }
                        j++;
                    }
                    i = j;
                }
                else
                {
                    // DOCTYPE 等 <! ... >
                    int j = i + 2;
                    while (j < length && html_content[j] != '>')
                        j++;
                    i = j;
                }
            }
            else // 处理开始标签 <div ...>
            {
                // 解析标签名
                int j = i + 1;
                while (html_content[j] != ' ' && html_content[j] != '>' && j < length)
                    j++;

                int tag_len = j - (i + 1);
                char *tag_name = (char *)malloc(tag_len + 1);
                strncpy(tag_name, html_content + i + 1, tag_len);
                tag_name[tag_len] = '\0';

                // 创建新结点
                Node *newNode = (Node *)malloc(sizeof(Node));
                InitNode(newNode, ELEMENT_NODE);
                newNode->tag_name = tag_name;

                // 解析属性 (id, class)
                while (html_content[j] != '>' && j < length)
                {
                    // 跳过空格
                    while (html_content[j] == ' ' && j < length)
                        j++;

                    if (html_content[j] == '>')
                        break;

                    // 读取属性名
                    int attr_start = j;
                    while (html_content[j] != '=' && html_content[j] != ' ' && html_content[j] != '>' && j < length)
                        j++;
                    int attr_len = j - attr_start;
                    char *attr_name = (char *)malloc(attr_len + 1);
                    strncpy(attr_name, html_content + attr_start, attr_len);
                    attr_name[attr_len] = '\0';

                    // 读取属性值
                    if (html_content[j] == '=')
                    {
                        j++; // 跳过 =
                        char quote = html_content[j];
                        if (quote == '"' || quote == '\'')
                        {
                            j++; // 跳过引号
                            int val_start = j;
                            while (html_content[j] != quote && j < length)
                                j++;
                            int val_len = j - val_start;
                            char *attr_val = (char *)malloc(val_len + 1);
                            strncpy(attr_val, html_content + val_start, val_len);
                            attr_val[val_len] = '\0';
                            // 存储属性，这里仅保存三种
                            if (strcmp(attr_name, "id") == 0)
                            {
                                newNode->id = attr_val;
                            }
                            else if (strcmp(attr_name, "href") == 0)
                            {
                                newNode->href = attr_val;
                            }
                            else if (strcmp(attr_name, "class") == 0)
                            {
                                // 解析 class 列表 (按空格分割，动态扩容)
                                int capacity = 10;
                                newNode->classes = (char **)malloc(capacity * sizeof(char *));
                                newNode->class_count = 0;

                                char *token = strtok(attr_val, " ");
                                while (token != NULL)
                                {
                                    if (newNode->class_count >= capacity)
                                    {
                                        capacity *= 2;
                                        newNode->classes = (char **)realloc(newNode->classes, capacity * sizeof(char *));
                                    }
                                    newNode->classes[newNode->class_count] = strdup(token);
                                    newNode->class_count++;
                                    token = strtok(NULL, " ");
                                }
                                free(attr_val);
                            }
                            else
                            {
                                free(attr_val); // 其他属性忽略
                            }
                            j++; // 跳过结束引号
                        }
                    }
                    free(attr_name);
                }

                // 寻找双亲，并加入树中
                Node *parent;
                GetTop(stack, parent);
                newNode->parent = parent;

                if (parent->children == nullptr)
                {
                    parent->children = newNode;
                }
                else
                {
                    // 找到最后一个兄弟
                    Node *sibling = parent->children;
                    while (sibling->next_sibling != nullptr)
                    {
                        sibling = sibling->next_sibling;
                    }
                    sibling->next_sibling = newNode;
                }

                // 自闭合标签处理 (如 <img ... /> 或 <br>)
                bool is_self_closing = false;
                if (html_content[j - 1] == '/')
                    is_self_closing = true;
                const char *self_closing_tags[] = {"img", "br", "hr", "input", "meta", "link", nullptr};
                for (int k = 0; self_closing_tags[k]; k++)
                {
                    if (strcmp(tag_name, self_closing_tags[k]) == 0)
                    {
                        is_self_closing = true;
                        break;
                    }
                }

                if (!is_self_closing)
                {
                    Push(stack, newNode);
                }

                i = j; // 更新索引
            }
        }
        else // 处理文本
        {
            int j = i;
            while (html_content[j] != '<' && j < length)
                j++;

            int text_len = j - i;
            if (text_len > 0)
            {
                // 保留文本结点的空白
                char *text_content = (char *)malloc(text_len + 1);
                strncpy(text_content, html_content + i, text_len);
                text_content[text_len] = '\0';

                // 创建文本结点
                Node *textNode = (Node *)malloc(sizeof(Node));
                InitNode(textNode, TEXT_NODE);
                textNode->text = text_content;
                // 找到双亲并加入树中
                Node *parent;
                GetTop(stack, parent);
                textNode->parent = parent;
                if (parent->children == nullptr)
                {
                    parent->children = textNode;
                }
                else
                {
                    Node *sibling = parent->children;
                    while (sibling->next_sibling != nullptr)
                    {
                        sibling = sibling->next_sibling;
                    }
                    sibling->next_sibling = textNode;
                }
            }
            i = j - 1; // 更新索引
        }
    }
    free(html_content);
    return OK;
}

// CSS选择器类
class CSS_selector
{
public:
    // 执行查询，逗号分隔的查询单独处理
    void select(Node *root, const char *query, NodeList *result)
    {
        if (!root || !query)
            return;
        char *buf = strdup(query);
        char *p = buf;
        char *start = p;
        while (*p)
        {
            if (*p == ',')
            {
                *p = '\0';
                execute_chain_query(root, start, result);
                start = p + 1;
            }
            p++;
        }
        execute_chain_query(root, start, result);
        free(buf);
    }

private:
    // 单元内部结构
    struct SelectorPart
    {
        char tag[64];
        char id[64];
        char classes[10][64];
        int class_count;
        char pseudo_class[32];   //  empty, first-child
        char pseudo_element[32]; //  before, after, first-letter, first-line
        char content[128];       // 用于 ::before("content") 和 ::after("content")

        SelectorPart()
        {
            tag[0] = '\0';
            id[0] = '\0';
            class_count = 0;
            pseudo_class[0] = '\0';
            pseudo_element[0] = '\0';
            content[0] = '\0';
        }
    };

    // 向 NodeList 添加结点
    void add_node(NodeList *list, Node *node)
    {
        if (!list || !node)
            return;
        for (int i = 0; i < list->count; ++i) // 查重
        {
            if (list->nodes[i] == node)
                return;
        }
        if (list->count >= list->capacity) // 扩容
        {
            int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
            Node **new_nodes = (Node **)realloc(list->nodes, new_cap * sizeof(Node *));
            if (!new_nodes)
                return;
            list->nodes = new_nodes;
            list->capacity = new_cap;
        }
        list->nodes[list->count++] = node;
    }

    // 解析一个单元的字符串
    void parse_selector_part(char *str, SelectorPart &sel)
    {
        sel.tag[0] = '\0';
        sel.id[0] = '\0';
        sel.class_count = 0;
        sel.pseudo_class[0] = '\0';
        sel.pseudo_element[0] = '\0';
        sel.content[0] = '\0';

        char *p = str;

        // 1. 解析 Tag
        if (*p == '#' || *p == '.' || *p == ':')
        {
            strcpy(sel.tag, "*");
        }
        else if (*p == '*')
        {
            strcpy(sel.tag, "*");
            p++;
        }
        else
        {
            char *start = p;
            while (*p && *p != '#' && *p != '.' && *p != ':')
                p++;
            int len = p - start;
            if (len > 63)
                len = 63;
            strncpy(sel.tag, start, len);
            sel.tag[len] = '\0';
        }

        // 2. 解析 ID, Class, Pseudo
        while (*p)
        {
            if (*p == '#')
            {
                p++;
                char *start = p;
                while (*p && *p != '#' && *p != '.' && *p != ':')
                    p++;
                int len = p - start;
                if (len > 63)
                    len = 63;
                strncpy(sel.id, start, len);
                sel.id[len] = '\0';
            }
            else if (*p == '.')
            {
                p++;
                char *start = p;
                while (*p && *p != '#' && *p != '.' && *p != ':')
                    p++;
                int len = p - start;
                if (len > 63)
                    len = 63;
                if (sel.class_count < 10)
                {
                    strncpy(sel.classes[sel.class_count], start, len);
                    sel.classes[sel.class_count][len] = '\0';
                    sel.class_count++;
                }
            }
            else if (*p == ':')
            {
                p++;
                if (*p == ':') // Pseudo-element (::)
                {
                    p++;
                    char *start = p;
                    while (*p && *p != '#' && *p != '.' && *p != ':' && *p != '(')
                        p++;
                    int len = p - start;
                    if (len > 31)
                        len = 31;
                    strncpy(sel.pseudo_element, start, len);
                    sel.pseudo_element[len] = '\0';

                    // 读取 ("content")
                    if (*p == '(')
                    {
                        p++; // 跳过 (
                        // 检查是否有引号
                        if (*p == '"' || *p == '\'')
                            p++;
                        char *c_start = p;
                        while (*p && *p != '"' && *p != '\'' && *p != ')')
                            p++;
                        int c_len = p - c_start;
                        if (c_len > 127)
                            c_len = 127;
                        strncpy(sel.content, c_start, c_len);
                        sel.content[c_len] = '\0';
                        // 跳过剩余直到 )
                        while (*p && *p != ')')
                            p++;
                        if (*p == ')')
                            p++;
                    }
                }
                else // Pseudo-class (:)
                {
                    char *start = p;
                    while (*p && *p != '#' && *p != '.' && *p != ':')
                        p++;
                    int len = p - start;
                    if (len > 31)
                        len = 31;
                    strncpy(sel.pseudo_class, start, len);
                    sel.pseudo_class[len] = '\0';
                }
            }
            else
            {
                p++;
            }
        }
        if (strlen(sel.tag) == 0)
            strcpy(sel.tag, "*");
    }

    // 查找子树中第一个文本结点
    Node *find_first_text_node(Node *node)
    {
        if (!node)
            return nullptr;
        if (node->type == TEXT_NODE)
            return node;
        Node *child = node->children;
        while (child)
        {
            Node *res = find_first_text_node(child);
            if (res)
                return res;
            child = child->next_sibling;
        }
        return nullptr;
    }

    // 检查结点与查询里包含的内容是否匹配
    bool match_part(Node *node, SelectorPart &sel)
    {
        if (node->type != ELEMENT_NODE)
            return false;

        // 1. Tag 匹配
        if (strcmp(sel.tag, "*") != 0 && strcmp(node->tag_name, sel.tag) != 0)
            return false;

        // 2. ID 匹配
        if (strlen(sel.id) > 0 && (!node->id || strcmp(node->id, sel.id) != 0))
            return false;

        // 3. Class 匹配
        for (int i = 0; i < sel.class_count; ++i)
        {
            bool found = false;
            if (node->classes)
            {
                for (int j = 0; j < node->class_count; ++j)
                {
                    if (strcmp(node->classes[j], sel.classes[i]) == 0)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                return false;
        }

        // 4. 伪类匹配 (:empty, :first-child)
        if (strlen(sel.pseudo_class) > 0)
        {
            if (strcmp(sel.pseudo_class, "empty") == 0)
            {
                // 检查是否有子结点
                if (node->children != nullptr)
                    return false;
            }
            else if (strcmp(sel.pseudo_class, "first-child") == 0)
            {
                // 检查是否是父结点的第一个元素子结点
                if (!node->parent)
                    return false;
                Node *child = node->parent->children;
                while (child && child->type != ELEMENT_NODE)
                    child = child->next_sibling;
                if (child != node)
                    return false;
            }
        }

        // 5. 伪元素匹配 (::before, ::after, ::first-letter, ::first-line)
        if (strlen(sel.pseudo_element) > 0)
        {
            // 选中宿主元素
            if (strcmp(sel.pseudo_element, "before") != 0 &&
                strcmp(sel.pseudo_element, "after") != 0 &&
                strcmp(sel.pseudo_element, "first-letter") != 0 &&
                strcmp(sel.pseudo_element, "first-line") != 0)
            {
                return false;
            }
        }

        return true;
    }

    // 递归遍历查找匹配
    void traverse_and_match(Node *node, SelectorPart &sel, NodeList *result)
    {
        if (!node)
            return;
        if (match_part(node, sel))
        {
            bool is_replacement_pseudo = false;

            // 处理 ::first-letter
            if (strcmp(sel.pseudo_element, "first-letter") == 0)
            {
                is_replacement_pseudo = true;
                Node *firstTextNode = find_first_text_node(node);
                if (firstTextNode && firstTextNode->text)
                {
                    char *text = firstTextNode->text;
                    // 跳过空白
                    char *start = text;
                    while (*start && isspace((unsigned char)*start))
                        start++;

                    if (*start)
                    {
                        // 获取第一个字符，by Gemini 3 Pro
                        int len = 1;
                        unsigned char c = (unsigned char)*start;
                        if (c >= 0xF0)
                            len = 4;
                        else if (c >= 0xE0)
                            len = 3;
                        else if (c >= 0xC0)
                            len = 2;

                        char *letter = (char *)malloc(len + 1);
                        strncpy(letter, start, len);
                        letter[len] = '\0';

                        Node *newNode = (Node *)malloc(sizeof(Node));
                        InitNode(newNode, TEXT_NODE);
                        newNode->text = letter;
                        add_node(result, newNode);
                    }
                }
            }
            // 处理 ::first-line
            else if (strcmp(sel.pseudo_element, "first-line") == 0)
            {
                is_replacement_pseudo = true;
                Node *firstTextNode = find_first_text_node(node);
                if (firstTextNode && firstTextNode->text)
                {
                    char *text = firstTextNode->text;

                    // 跳过空白
                    char *start = text;
                    while (*start && isspace((unsigned char)*start))
                        start++;

                    if (*start)
                    {
                        // 获取第一行 (从非空白字符开始，直到下一个换行符)
                        char *newline = strchr(start, '\n');
                        int len = newline ? (newline - start) : strlen(start);

                        if (len > 0)
                        {
                            char *line = (char *)malloc(len + 1);
                            strncpy(line, start, len);
                            line[len] = '\0';

                            Node *newNode = (Node *)malloc(sizeof(Node));
                            InitNode(newNode, TEXT_NODE);
                            newNode->text = line;
                            add_node(result, newNode);
                        }
                    }
                }
            }

            // 处理 ::before 和 ::after 的内容插入
            if (strlen(sel.content) > 0)
            {
                if (strcmp(sel.pseudo_element, "before") == 0)
                {
                    // 在第一个子结点前插入文本结点
                    Node *newText = (Node *)malloc(sizeof(Node));
                    InitNode(newText, TEXT_NODE);
                    newText->text = strdup(sel.content);
                    newText->parent = node;
                    newText->next_sibling = node->children;
                    node->children = newText;
                }
                else if (strcmp(sel.pseudo_element, "after") == 0)
                {
                    // 在最后一个子结点后插入文本结点
                    Node *newText = (Node *)malloc(sizeof(Node));
                    InitNode(newText, TEXT_NODE);
                    newText->text = strdup(sel.content);
                    newText->parent = node;

                    if (node->children == nullptr)
                    {
                        node->children = newText;
                    }
                    else
                    {
                        Node *curr = node->children;
                        while (curr->next_sibling)
                            curr = curr->next_sibling;
                        curr->next_sibling = newText;
                    }
                }
            }

            if (!is_replacement_pseudo)
            {
                add_node(result, node);
            }
        }
        traverse_and_match(node->children, sel, result);
        traverse_and_match(node->next_sibling, sel, result);
    }

    // 执行单个查询
    void execute_chain_query(Node *root, char *query, NodeList *result)
    {
        // 分成五个，Selector, ' ', '>', '+', '~'
        // 即，名词——Selector，动词——' ', '>', '+', '~'
        struct Token
        {
            char str[128];
            int type; // 0=sel, 1=' ', 2='>', 3='+', 4='~'
        } tokens[20];
        int count = 0;
        char *p = query;

        while (*p)
        {
            while (*p == ' ')
                p++;
            if (!*p)
                break;
            if (*p == '>' || *p == '+' || *p == '~')
            {
                tokens[count].type = (*p == '>') ? 2 : (*p == '+' ? 3 : 4);
                tokens[count].str[0] = '\0';
                count++;
                p++;
            }
            else
            {
                char *start = p;
                while (*p && *p != ' ' && *p != '>' && *p != '+' && *p != '~')
                    p++;
                int len = p - start;
                if (len > 127)
                    len = 127;
                strncpy(tokens[count].str, start, len);
                tokens[count].str[len] = '\0';
                tokens[count].type = 0;
                count++;
                if (*p == ' ')
                {
                    char *next = p;
                    while (*next == ' ')
                        next++;
                    if (*next && *next != '>' && *next != '+' && *next != '~')
                    {
                        tokens[count].type = 1;
                        tokens[count].str[0] = '\0';
                        count++;
                    }
                }
            }
        }

        if (count == 0) // 查询为空
            return;

        NodeList current = {nullptr, 0, 0};

        // 寻找匹配的第一波结点
        if (tokens[0].type == 0)
        {
            SelectorPart sel;
            parse_selector_part(tokens[0].str, sel);
            traverse_and_match(root->children, sel, &current);
        }
        // 后续依据动词去处理当前列表
        for (int i = 1; i < count; ++i)
        {
            if (tokens[i].type == 0)
                continue;
            int comb = tokens[i].type;
            if (i + 1 >= count || tokens[i + 1].type != 0)
                break;

            SelectorPart next_sel;
            parse_selector_part(tokens[i + 1].str, next_sel); // 读下一个词
            i++;

            NodeList next_list = {nullptr, 0, 0};
            for (int j = 0; j < current.count; ++j)
            {
                Node *ctx = current.nodes[j];
                if (comb == 1)
                { // ' '
                    traverse_and_match(ctx->children, next_sel, &next_list);
                }
                else if (comb == 2)
                { // '>'
                    Node *child = ctx->children;
                    while (child)
                    {
                        if (match_part(child, next_sel))
                            add_node(&next_list, child);
                        child = child->next_sibling;
                    }
                }
                else if (comb == 3)
                { // '+'
                    Node *sib = ctx->next_sibling;
                    while (sib && sib->type != ELEMENT_NODE)
                        sib = sib->next_sibling;

                    if (sib && match_part(sib, next_sel))
                    {
                        add_node(&next_list, sib);
                    }
                }
                else if (comb == 4)
                { // '~'
                    Node *sib = ctx->next_sibling;
                    while (sib)
                    {
                        if (match_part(sib, next_sel))
                            add_node(&next_list, sib);
                        sib = sib->next_sibling;
                    }
                }
            }
            if (current.nodes)
                free(current.nodes);
            current = next_list; // 更新当前的列表
            if (current.count == 0)
                break;
        }
        for (int i = 0; i < current.count; ++i)
            add_node(result, current.nodes[i]);
        if (current.nodes)
            free(current.nodes);
    }
};

// XPath 选择器类，将lab2的Xpath代码实现直接搬到这里，Gemini 3 Pro进行适配
class XPath_selector
{
public:
    void select(Node *root, const char *xpath, NodeList *result)
    {
        if (!root || !xpath)
            return;

        // 初始上下文：仅包含 root
        NodeList current = {nullptr, 0, 0};
        add_node(&current, root);

        char *p = (char *)xpath;

        // 如果不以 / 开头，默认为相对路径（子结点查找）
        // 如果以 / 开头，则从当前上下文开始（这里上下文就是 root）
        // root 是虚拟根结点，所以 /html 实际上是查找 root 的子结点 html

        while (*p)
        {
            bool recursive = false;
            if (*p == '/')
            {
                p++;
                if (*p == '/')
                {
                    recursive = true;
                    p++;
                }
            }

            // 解析标签名
            char tag[64] = {0};
            char *start = p;
            while (*p && *p != '/' && *p != '[')
                p++;
            int len = p - start;
            if (len > 63)
                len = 63;
            strncpy(tag, start, len);
            tag[len] = '\0';

            // 解析谓语 [...]
            char attr[64] = {0};
            char val[64] = {0};
            int index = 0; // 1-based index

            if (*p == '[')
            {
                p++;
                if (*p == '@') // 属性 [@id='val']
                {
                    p++; // skip @
                    char *a_start = p;
                    while (*p && *p != '=')
                        p++;
                    int a_len = p - a_start;
                    strncpy(attr, a_start, a_len);
                    attr[a_len] = '\0';

                    if (*p == '=')
                        p++;
                    char quote = *p;
                    if (quote == '\'' || quote == '"')
                    {
                        p++;
                        char *v_start = p;
                        while (*p && *p != quote)
                            p++;
                        int v_len = p - v_start;
                        strncpy(val, v_start, v_len);
                        val[v_len] = '\0';
                        p++; // skip quote
                    }
                }
                else if (isdigit((unsigned char)*p)) // 索引 [1]
                {
                    index = atoi(p);
                    while (isdigit((unsigned char)*p))
                        p++;
                }
                while (*p && *p != ']')
                    p++;
                if (*p == ']')
                    p++;
            }

            // 执行当前步
            NodeList next = {nullptr, 0, 0};

            // 对当前集合中的每个结点进行查找
            for (int i = 0; i < current.count; ++i)
            {
                Node *ctx = current.nodes[i];
                // 查找匹配的结点
                find_matches(ctx, tag, recursive, attr, val, index, &next);
            }

            // 更新当前集合
            if (current.nodes)
                free(current.nodes);
            current = next;

            if (current.count == 0)
                break;
        }

        // 输出结果
        for (int i = 0; i < current.count; ++i)
            add_node(result, current.nodes[i]);
        if (current.nodes)
            free(current.nodes);
    }

private:
    void add_node(NodeList *list, Node *node)
    {
        if (!list || !node)
            return;
        for (int i = 0; i < list->count; ++i)
            if (list->nodes[i] == node)
                return;

        if (list->count >= list->capacity)
        {
            int new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
            Node **new_nodes = (Node **)realloc(list->nodes, new_cap * sizeof(Node *));
            if (!new_nodes)
                return;
            list->nodes = new_nodes;
            list->capacity = new_cap;
        }
        list->nodes[list->count++] = node;
    }

    bool check_node(Node *node, const char *tag, const char *attr, const char *val)
    {
        // 支持 text() 选择器
        if (strcmp(tag, "text()") == 0)
        {
            return node->type == TEXT_NODE;
        }

        if (node->type != ELEMENT_NODE)
            return false;

        // 检查标签名
        if (strcmp(tag, "*") != 0 && strcmp(node->tag_name, tag) != 0)
            return false;

        // 检查属性
        if (strlen(attr) > 0)
        {
            if (strcmp(attr, "id") == 0)
            {
                if (!node->id || strcmp(node->id, val) != 0)
                    return false;
            }
            else if (strcmp(attr, "class") == 0)
            {
                // 简化：只要包含该 class 即可
                bool found = false;
                if (node->classes)
                {
                    for (int k = 0; k < node->class_count; ++k)
                        if (strcmp(node->classes[k], val) == 0)
                            found = true;
                }
                if (!found)
                    return false;
            }
            else if (strcmp(attr, "href") == 0)
            {
                if (!node->href || strcmp(node->href, val) != 0)
                    return false;
            }
            else
            {
                return false; // 不支持的属性
            }
        }
        return true;
    }

    void find_matches(Node *ctx, const char *tag, bool recursive, const char *attr, const char *val, int index, NodeList *result)
    {
        if (!ctx)
            return;

        // 收集所有候选子结点（用于处理索引）
        // 如果是递归，逻辑稍微复杂一点：通常 XPath 的 [n] 是针对父结点的子结点列表的位置
        // 这里简化实现：
        // 1. 如果 recursive=false (即 /tag)，遍历 ctx->children，找到第 index 个匹配的
        // 2. 如果 recursive=true (即 //tag)，遍历 ctx 的整棵子树，对每个结点，如果它匹配 tag/attr，加入结果。
        //    注意：标准 XPath //tag[1] 意思是 "所有 tag 元素，且它是其父元素的第一个 tag 子元素"

        if (recursive)
        {
            // 递归遍历子树
            Node *child = ctx->children;
            while (child)
            {
                // 检查当前 child 是否符合要求
                bool match = check_node(child, tag, attr, val);
                if (match)
                {
                    if (index > 0)
                    {
                        // 检查它是否是父结点的第 index 个匹配 tag 的子结点
                        int count = 0;
                        Node *sib = child->parent->children;
                        while (sib)
                        {
                            if (check_node(sib, tag, "", "")) // 仅按 tag 计数
                                count++;
                            if (sib == child)
                                break;
                            sib = sib->next_sibling;
                        }
                        if (count == index)
                            add_node(result, child);
                    }
                    else
                    {
                        add_node(result, child);
                    }
                }

                // 继续递归
                find_matches(child, tag, true, attr, val, index, result);

                child = child->next_sibling;
            }
        }
        else
        {
            // 仅查找直接子结点
            int count = 0;
            Node *child = ctx->children;
            while (child)
            {
                if (check_node(child, tag, attr, val))
                {
                    count++;
                    if (index == 0 || count == index)
                    {
                        add_node(result, child);
                    }
                }
                child = child->next_sibling;
            }
        }
    }
};

// 结果处理类
class ResultHandler
{
public:
    // 去除text的首尾空白
    static char *trim_string(const char *str)
    {
        if (!str)
            return strdup("");
        const char *start = str;
        while (*start && isspace((unsigned char)*start))
            start++;
        const char *end = str + strlen(str) - 1;
        while (end > start && isspace((unsigned char)*end))
            end--;
        int len = end - start + 1;
        if (len <= 0)
            return strdup("");
        char *res = (char *)malloc(len + 1);
        strncpy(res, start, len);
        res[len] = '\0';
        return res;
    }

    // 获取结点的 innerText，每个文本结点之间用换行符隔开
    static char *get_innerText(Node *node)
    {
        if (!node)
            return strdup("");

        if (node->type == TEXT_NODE)
        {
            return trim_string(node->text);
        }
        // 元素结点，拼接所有子结点的文本
        int capacity = 256;
        char *result = (char *)malloc(capacity);
        result[0] = '\0';

        Node *child = node->children;
        bool first = true;
        while (child)
        {
            char *child_text = get_innerText(child);
            if (strlen(child_text) > 0)
            {
                int len = strlen(child_text);
                int needed = strlen(result) + len + 2;
                if (needed > capacity)
                {
                    capacity = needed * 2;
                    result = (char *)realloc(result, capacity);
                }

                if (!first)
                {
                    strcat(result, "\n");
                }
                strcat(result, child_text);
                first = false;
            }
            free(child_text);
            child = child->next_sibling;
        }
        return result;
    }

    // 获取结点的 outerHTML，直接输出源码
    static char *get_outerHTML(Node *node)
    {
        if (!node)
            return strdup("");

        if (node->type == TEXT_NODE)
        {
            return node->text ? strdup(node->text) : strdup("");
        }

        // 估算长度
        int capacity = 256;
        char *result = (char *)malloc(capacity);
        result[0] = '\0';
        // 开始标签
        char tag_buf[256];
        sprintf(tag_buf, "<%s", node->tag_name);
        if (strlen(result) + strlen(tag_buf) + 1 > capacity)
        {
            capacity *= 2;
            result = (char *)realloc(result, capacity);
        }
        strcat(result, tag_buf);
        if (node->id)
        {
            char id_buf[128];
            sprintf(id_buf, " id=\"%s\"", node->id);
            if (strlen(result) + strlen(id_buf) + 1 > capacity)
            {
                capacity += 128;
                result = (char *)realloc(result, capacity);
            }
            strcat(result, id_buf);
        }
        if (node->class_count > 0)
        {
            if (strlen(result) + 10 + 1 > capacity)
            {
                capacity += 128;
                result = (char *)realloc(result, capacity);
            }
            strcat(result, " class=\"");
            for (int i = 0; i < node->class_count; ++i)
            {
                if (strlen(result) + strlen(node->classes[i]) + 2 > capacity)
                {
                    capacity *= 2;
                    result = (char *)realloc(result, capacity);
                }
                strcat(result, node->classes[i]);
                if (i < node->class_count - 1)
                    strcat(result, " ");
            }
            strcat(result, "\"");
        }
        if (node->href)
        {
            char href_buf[256];
            sprintf(href_buf, " href=\"%s\"", node->href);
            if (strlen(result) + strlen(href_buf) + 1 > capacity)
            {
                capacity += 256;
                result = (char *)realloc(result, capacity);
            }
            strcat(result, href_buf);
        }

        strcat(result, ">");
        // 子结点内容
        Node *child = node->children;
        while (child)
        {
            char *child_html = get_outerHTML(child);
            int len = strlen(child_html);
            while (strlen(result) + len + 1 > capacity)
            {
                capacity *= 2;
                result = (char *)realloc(result, capacity);
            }
            strcat(result, child_html);
            free(child_html);
            child = child->next_sibling;
        }
        // 结束标签
        char end_tag[64];
        sprintf(end_tag, "</%s>", node->tag_name);
        while (strlen(result) + strlen(end_tag) + 1 > capacity)
        {
            capacity *= 2;
            result = (char *)realloc(result, capacity);
        }
        strcat(result, end_tag);
        return result;
    }

    // 获取 A 标签的 href
    static char *get_href(Node *node)
    {
        if (node && node->type == ELEMENT_NODE && node->href)
        {
            return strdup(node->href);
        }
        return nullptr;
    }
};

int main()
{
    // 设置控制台为UTF-8编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    char path[256];
    Node *root = nullptr;

    while (true)
    {
        cout << "================================================" << endl;
        cout << "                      主菜单                    " << endl;
        cout << "================================================" << endl;
        cout << "[1] 加载 HTML 文件" << endl;
        cout << "[2] 退出程序" << endl;
        cout << "请选择: ";

        int choice;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        if (choice == 2)
            break;
        if (choice == 1)
        {
            cout << "请输入文件路径: ";
            cin >> path;
            // 释放旧树
            if (root)
            {
                FreeTree(root);
                root = nullptr;
            }
            if (load_html(path, root) == OK)
            {
                cout << "\n>>> 文件加载成功! <<<" << endl;
                cin.ignore(); // 清除换行符

                while (true)
                {
                    cout << "\n------------------------------------------------" << endl;
                    cout << "[1] CSS 选择器" << endl;
                    cout << "[2] XPath 选择器" << endl;
                    cout << "[3] 返回主菜单" << endl;
                    cout << "请选择: ";

                    int sub_choice;
                    if (!(cin >> sub_choice))
                    {
                        cin.clear();
                        cin.ignore(10000, '\n');
                        continue;
                    }
                    cin.ignore(); // 清除换行符

                    if (sub_choice == 3)
                        break;

                    if (sub_choice == 1 || sub_choice == 2)
                    {
                        char query[256];
                        if (sub_choice == 1)
                        {
                            cout << "\n------------------------------------------------" << endl;
                            cout << "可用功能：" << endl
                                 << "标签选择器、ID选择器、类选择器、" << endl
                                 << "后代选择器、子元素选择器、" << endl
                                 << "相邻兄弟选择器、通用兄弟选择器、" << endl
                                 << "伪类(:empty, :first-child)、" << endl
                                 << "伪元素(::before, ::after, ::first-letter, ::first-line)" << endl;
                            cout << "------------------------------------------------" << endl;
                            cout << ">> 请输入 CSS 选择器: ";
                        }
                        else
                            cout << ">> 请输入 XPath 选择器: ";
                        cin.getline(query, 256);
                        NodeList current_list = {nullptr, 0, 0};
                        if (sub_choice == 1)
                        {
                            CSS_selector selector;
                            selector.select(root, query, &current_list);
                        }
                        else
                        {
                            XPath_selector selector;
                            selector.select(root, query, &current_list);
                        }
                        // 结果操作循环
                        while (true)
                        {
                            cout << "\n>>> 当前结果集 (" << current_list.count << " 个结点) <<<" << endl;
                            for (int i = 0; i < current_list.count; ++i)
                            {
                                Node *node = current_list.nodes[i];
                                cout << "[" << i + 1 << "] ";
                                if (node->type == ELEMENT_NODE)
                                {
                                    cout << "<" << (node->tag_name ? node->tag_name : "unknown");
                                    if (node->id)
                                        cout << " id='" << node->id << "'";
                                    if (node->class_count > 0)
                                        cout << " class='" << node->classes[0] << "...'";
                                    cout << ">";
                                }
                                else
                                {
                                    cout << "TextNode";
                                }
                                cout << endl;
                            }
                            if (current_list.count == 0)
                            {
                                cout << "结果为空，按回车返回..." << endl;
                                cin.get();
                                if (current_list.nodes)
                                    free(current_list.nodes);
                                break;
                            }
                            cout << "\n[操作菜单]" << endl;
                            cout << "[1] 选择结点 k 进行子查询" << endl;
                            cout << "[2] 查看结点 k 的 InnerText" << endl;
                            cout << "[3] 查看结点 k 的 OuterHTML" << endl;
                            cout << "[4] 列出所有结点的 href 属性" << endl;
                            cout << "[5] 放弃当前结果，开始新查询" << endl;
                            cout << "请选择: ";
                            int op_choice;
                            if (!(cin >> op_choice))
                            {
                                cin.clear();
                                cin.ignore(10000, '\n');
                                continue;
                            }
                            cin.ignore();
                            if (op_choice == 5)
                            {
                                if (current_list.nodes)
                                    free(current_list.nodes);
                                break;
                            }
                            if (op_choice == 4)
                            {
                                cout << "\n--- All Hrefs ---\n";
                                for (int i = 0; i < current_list.count; ++i)
                                {
                                    char *href = ResultHandler::get_href(current_list.nodes[i]);
                                    if (href)
                                    {
                                        cout << href << endl;
                                        free(href);
                                    }
                                }
                                cout << "-----------------\n";
                                cout << "按回车继续...";
                                cin.get();
                                continue;
                            }
                            if (op_choice >= 1 && op_choice <= 3)
                            {
                                int k;
                                cout << "请输入 k (1-" << current_list.count << "): ";
                                if (!(cin >> k))
                                {
                                    cin.clear();
                                    cin.ignore(10000, '\n');
                                    continue;
                                }
                                cin.ignore();
                                if (k < 1 || k > current_list.count)
                                {
                                    cout << "无效的索引!" << endl;
                                    continue;
                                }
                                Node *target = current_list.nodes[k - 1];
                                if (op_choice == 1)
                                {
                                    cout << "------------------------------------------------" << endl;
                                    cout << "请选择子查询类型:\n[1] CSS 选择器\n[2] XPath 选择器\n请选择: ";
                                    int sub_type;
                                    if (!(cin >> sub_type))
                                    {
                                        cin.clear();
                                        cin.ignore(10000, '\n');
                                        continue;
                                    }
                                    cin.ignore();

                                    char sub_q[256];
                                    if (sub_type == 1)
                                        cout << ">> 请输入子查询 CSS 选择器 (在结点 " << k << " 下): ";
                                    else
                                        cout << ">> 请输入子查询 XPath 选择器 (在结点 " << k << " 下): ";

                                    cin.getline(sub_q, 256);
                                    NodeList new_list = {nullptr, 0, 0};
                                    if (sub_type == 1)
                                    {
                                        CSS_selector selector;
                                        selector.select(target, sub_q, &new_list);
                                    }
                                    else
                                    {
                                        XPath_selector selector;
                                        selector.select(target, sub_q, &new_list);
                                    }
                                    // 更新结果集
                                    if (current_list.nodes)
                                        free(current_list.nodes);
                                    current_list = new_list;
                                    cout << "结果集已更新。" << endl;
                                }
                                else if (op_choice == 2)
                                {
                                    char *text = ResultHandler::get_innerText(target);
                                    cout << "\n--- InnerText (Node " << k << ") ---\n";
                                    cout << text << "\n-----------------------------\n";
                                    free(text);
                                    cout << "按回车继续...";
                                    cin.get();
                                }
                                else if (op_choice == 3)
                                {
                                    char *html = ResultHandler::get_outerHTML(target);
                                    cout << "\n--- OuterHTML (Node " << k << ") ---\n";
                                    cout << html << "\n-----------------------------\n";
                                    free(html);
                                    cout << "按回车继续...";
                                    cin.get();
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                cout << "文件打开失败，请检查路径。" << endl;
            }
        }
    }
    return 0;
}