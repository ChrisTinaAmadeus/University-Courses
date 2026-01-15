#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int n, m;
int f[10];
int pai[10];
long long count1, count2;
long long power(int n)
{
    long long sum = 1;
    while (n > 0)
    {
        sum *= 10;
        n--;
    }
    return sum;
}
long long num(int l, int r)//l是左端第一个数,r是一共多少位
{
    long long sum = 0;
    int wei = 1;
    for (int i = l; wei <= r; i++)
    {
        sum += power(r - wei) * pai[i];
        wei++;
    }
    return sum;
}
void countall(int k)
{
    if (k == m + 1)
    {
        for (int i = 1; i <= m - 2; i++)
        {
            for (int j = 1; j <= m - 2; j++)
            {
                if (i + j >= m)
                    continue;
                int k = m - i - j;
                long long a = num(1, i);
                long long b = num(i + 1, j), c = num(i + j + 1, k);
                if (a + b / c == n && b % c == 0)
                    count1++;
                if (a + b % c == n)
                    count2++;
            }
        }
        return;
    }
    for (int i = 1; i <= m; i++)
    {
        if (f[i] != 1)
        {
            f[i] = 1;
            pai[k] = i;
            countall(k + 1);
            pai[k] = 0;
            f[i] = 0;
        }
    }
}
int main()
{
    scanf("%d%d", &n, &m);
    countall(1);
    printf("%lld %lld", count1, count2);
    return 0;
}