#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
struct rock
{
    int v;
    int e;
};
rock rocks[20000];
rock dp[20000];
int main()
{
    int pack, num, energy;
    scanf("%d%d%d", &pack, &num, &energy);
    for (int i = 0; i < num;i++)
    {
        scanf("%d%d", &rocks[i].v, &rocks[i].e);
        for (int j = energy; j >= rocks[i].e;j--)
        {
            dp[j].v=max( dp[j - rocks[i].e].v+rocks[i].v,dp[j].v);
        }
    }
    bool judge = false;
    for (int i = 1; i <= energy;i++)
    {
        if(dp[i].v>=pack)
        {
            printf("%d", energy - i);
            judge = true;
            break;
        }
    }
    if(!judge)
    {
        printf("Impossible");
    }
    return 0;
}