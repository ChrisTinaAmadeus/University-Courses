#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
struct juzi
{
    int chang[80];
    int pro;
};

int main() 
{
    juzi all[5001]={};
    int V,l,k;
    scanf("%d%d%d",&V,&l,&k);
    int rij[80][80]={};
    for(int i=0;i<=V;i++)
        for(int j=1;j<=V;j++)
            scanf("%d",&rij[i][j]);
    for(int i=1;i<=V;i++)
    {
        all[i].pro+=rij[0][i];
        all[i].chang[1]=i;
    }
    juzi temp;
    for(int i=1;i<=V;i++)
        for(int j=1;j<V;j++)
        {
            if(all[j].pro<all[j+1].pro)
            {
                temp=all[j];
                all[j]=all[j+1];
                all[j+1]=temp;
            }
        }
    for(int i=k+1;i<=V;i++)
    all[i]=all[5000];
    for(int L=2;L<=l;L++)
    {
        int initial=1;
        for(int j=k+1;j<=k+k*V;j+=V)
        {
            int num=1;
            for(int m=j;m<j+V;m++)
            {
                all[m]=all[initial];
                all[m].chang[L]=num;
                all[m].pro+=rij[all[m].chang[L-1]][num];
                num++;
            }
            initial++;
        }
        for(int t=1;t<=k+k*V;t++)
            for(int j=1;j<k+k*V;j++)
            {
                if(all[j].pro<all[j+1].pro)
                {
                    temp=all[j];
                    all[j]=all[j+1];
                    all[j+1]=temp;
                }
            }
        for(int i=k+1;i<=k+k*V;i++)
        all[i]=all[5000];
    }
    for(int i=1;i<=k;i++)
    {
        for(int j=1;j<=l;j++)
        {
            printf("%d ",all[i].chang[j]);
        }
        printf("\n");
    }
    return 0;
}