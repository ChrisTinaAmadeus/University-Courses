#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 40
#define M 300000

char allRes[M][N] = {0}; // 保存所有结果
char szRes[N] = {0};     // 全局变量，保存n对括号组成的一个序列

void genParentheses(int n,int left,int right,int *s,int k)
{
        // 如果左右括号都已经达到 n，表示生成了一个合法的括号组合
        if (left == n && right == n)
        {
            szRes[k] = '\0'; // 结束字符串
            strcpy(allRes[*s], szRes);
            *s+=1;
            return;
        }

        // 可以添加左括号 '('，前提是左括号的数量 < n
        if (left < n)
        {
            szRes[k] = '(';
            genParentheses(n, left + 1, right, s, k + 1);
        }

        // 可以添加右括号 ')', 前提是右括号的数量 < 左括号的数量
        if (right < left)
        {
            szRes[k] = ')';
            genParentheses(n, left, right + 1, s, k + 1);
        }
}

int main()
{
    int n, nRes = 0, i;

    scanf("%d", &n);
    genParentheses(n, 0,0,&nRes,0);

    printf("%d\n", nRes);
    for (i = 0; i < nRes; i++)
        printf("%s\n", allRes[i]);

    return 0;
}