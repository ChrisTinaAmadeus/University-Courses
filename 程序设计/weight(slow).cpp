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
int f[TYPE], F;
int countweighnum[TYPE];
void calculate(int n, int i, int k, int J, int m)
{
    if (allthings[i].weight > m)
        return;
    if (k == allthings[i].weight)
    {
        for (int p = 0; p < F; p++)
            printf("%d ", f[p]);
        printf("\n");
        allthings[i].ncount++;
        return;
    }
    for (int j = 0; j < n; j++)
    {
        if (j < J || countweighnum[j] == all[j].num)
            continue;
        if (k + all[j].weigh <= allthings[i].weight)
        {
            k += all[j].weigh;
            countweighnum[j]++;
            f[F] = all[j].weigh;
            F++;
            calculate(n, i, k, j, m);
            F--;
            f[F] = 0;
            countweighnum[j]--;
            k -= all[j].weigh;
        }
    }
    return;
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
    long long maxcount = 0;
    for (int i = 0; i < n; i++)
    {
        maxcount += (all[i].num * all[i].weigh);
    }
    for (int i = 0; i < m; i++)
        calculate(n, i, 0, 0, maxcount);
    qsort(allthings, m, sizeof(things), compare);
    for (int i = 0; i < m; i++)
    {
        printf("%d:%lld\n", allthings[i].weight, allthings[i].ncount);
    }
    return 0;
}