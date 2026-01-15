#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
struct cutnum
{
    int n;
    int k;
    bool judge;
    int mi[1000];
    long long record[1000];
};
cutnum all[20];
bool judgegood(int i)
{
    int num = 0;
    while(all[i].n>0)
    {
        int mi1 = 0;
        long long ji=1;
        for (int j = 0;;j++)
        {
            if(ji>=all[i].n)
            {
                if(ji==all[i].n)
                {
                    all[i].mi[num] = mi1;
                    all[i].record[num] = ji;
                    num++;
                    all[i].n -= ji;
                    break;
                }
                else
                {
                    all[i].mi[num] = mi1 - 1;
                    ji /= all[i].k;
                    all[i].record[num] = ji;
                    num++;
                    all[i].n -= ji;
                    break;
                }
            }
            ji *= all[i].k;
            mi1 += 1;
        }
    }
    for (int j = 0;j<num;j++)
    {
        if(all[i].mi[j]==0)
            return false;
        if(j>0&&all[i].mi[j]==all[i].mi[j-1])
            return false;
    }
    return true;
}
int main()
{
    int T;
    scanf("%d", &T);
    for (int i = 0; i < T;i++)
    {
        scanf("%d%d", &all[i].n, &all[i].k);
        
    }
    for (int i = 0; i < T;i++)
    {
        all[i].judge=judgegood(i);
        if(!all[i].judge)
            printf("-1\n");
        else
        {
            for (int j = 0;;j++)
            {
                if(all[i].record[j]==0)
                    break;
                printf("%lld ", all[i].record[j]);
            }
            printf("\n");
        }
    }
        return 0;
}