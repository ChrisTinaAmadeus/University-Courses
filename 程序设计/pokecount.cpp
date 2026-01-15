#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
struct card
{
    char c[5];
};
struct poke
{
    int num;
    card total[55];
    int ncount=51;
};
int main() 
{
    int T;
    poke all[10]={};
    scanf("%d",&T);
    for(int i=0;i<T;i++)
    {
        scanf("%d",&all[i].num);
        for(int j=0;j<all[i].num;j++)
        {
            scanf("%s",all[i].total[j].c);
            if(j>=1)
            {
                int judge=1;
                for(int k=j-1;k>=0&&judge;k--)
                {
                    if(strcmp(all[i].total[j].c,all[i].total[k].c)==0)
                    judge=0;
                }
                if(judge)all[i].ncount--;
            }
        }
    }
    for(int i=0;i<T;i++)
    {
        printf("%d\n",all[i].ncount);
    }
    return 0;
}