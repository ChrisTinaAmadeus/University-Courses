#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <iostream>
using namespace std;
bool judgeprime(int n)
{
    if (n % 2 == 0)
        return false;
    for (int i = 3; i <= int(sqrt(n)); i += 2)
    {
        if (n % i == 0)
            return false;
    }
    return true;
}
bool judgehuiwen(int n)
{
    int N = n;
    int allnum[20] = {};
    int i = 0;
    while (n > 0)
    {
        allnum[i++] = n % 10;
        n /= 10;
    }
    for (int j = 0; j < i / 2; j++)
    {
        if (allnum[j] != allnum[i - 1 - j])
            return false;
    }
    if (i % 2 == 0 && N != 11) // 有偶数位的回文数（除了11）必然不是质数
        return false;
    return true;
}
int main()
{
    int a, b;
    scanf("%d%d", &a, &b);
    b = min(9999999, b); // 题设范围是到一亿，八位数都不可能是回文质数
    for (int i = a; i <= b; i++)
    {
        if (judgehuiwen(i))
        {
            if (judgeprime(i))
                printf("%d\n", i);
        }
    }
    return 0;
}