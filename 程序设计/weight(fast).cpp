#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#define SIZEMAX 300
#define TYPE 20
using namespace std;
struct weight
{
    int num;
    int weigh;
};
weight all[TYPE];
struct things
{
    long long ncount;
    int weight;
};
things allthings[SIZEMAX];
int countweighnum[TYPE];
long long dp[10100];
void calculate(int n)//背包问题，dp动态规划
{
    for (int i = 0; i < n; i++)//物品遍历
    {
        for (int j = 10000; j >= all[i].weigh; j--)//背包容量
        {
            for (int k = 1; k <= all[i].num && j - all[i].weigh * k >=0; k++)//条件遍历：数量等
            {
                dp[j]+=dp[j - all[i].weigh * k];
            }
        }
    }
}

int compare(const void *a, const void *b)
{
    int n = (*(things *)b).ncount - (*(things *)a).ncount;
    if (n != 0)
        return n;
    else
        return (*(things *)a).weight - (*(things *)b).weight;
}
int main()
{
    dp[0] = 1;
    int n, m;
    scanf("%d%d", &n, &m);
    // 砝码种类数n，物体数量m
    for (int i = 0; i < n; i++)
    {
        scanf("%d", &all[i].weigh);
    }
    for (int i = 0; i < n; i++)
        scanf("%d", &all[i].num);
    for (int i = 0; i < m; i++)
        scanf("%d", &allthings[i].weight);
    calculate(n);
    for (int i = 0; i < m; i++)
    {
        allthings[i].ncount = dp[allthings[i].weight];
    }
    qsort(allthings, m, sizeof(things), compare);
    for (int i = 0; i < m; i++)
    {
        printf("%d:%lld\n", allthings[i].weight, allthings[i].ncount);
    }
    return 0;
}