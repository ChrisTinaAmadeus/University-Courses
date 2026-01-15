#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
#define MAX 1000000
struct recordchushu
{
    int order;
    int ncount;
};
recordchushu all[MAX];
int main()
{
    int N, D;
    scanf("%d%d", &N, &D);
    if(N%D==0)
    {
        printf("%.1f", N * 1.0 / D * 1.0);
    }
    else 
    {
        printf("%d.", N / D);
        N %= D;
        int record[1000] = {};
        int length = 0,findxunhuan=0;
        bool judgezheng = false, judgexunhuan = false;
        for (int i = 1;;i++)
        {
            N *= 10;
            all[N].ncount += 1;
            if(all[N].ncount==2)
            {
                length = i - all[N].order;
                judgexunhuan = true;
                findxunhuan = all[N].order;
                break;
            }
            all[N].order = i;
            record[i] = N / D;
            if(N%D==0)
            {
                judgezheng = true;
                length = i;
                break;
            }
            N %= D;
        }
        if(judgezheng)
        {
            for (int i = 1;i<=length;i++)
                printf("%d", record[i]);
        }
        if(judgexunhuan)
        {
            for (int i = 1; i < findxunhuan;i++)
                printf("%d", record[i]);
            printf("(");
            for (int i = findxunhuan; i <= findxunhuan + length-1;i++)
            {
                printf("%d", record[i]);
            }
            printf(")");
        }
    }
    return 0;
}