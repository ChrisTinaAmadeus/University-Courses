#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
struct customer
{
    int num;
    int item[120];
};
int main() 
{
    int n,ncount=0;
    customer all[30]={};
    scanf("%d",&n);
    int shop[120]={};
    for(int i=0;i<n;i++)
    {
        scanf("%d",&all[i].num);
        for(int j=0;j<all[i].num;j++)
        {
            scanf("%d",&all[i].item[j]);
        }
    }
    for(int k=0;k<n;k++)
        for(int t=0;t<all[k].num;t++)
        {
            int judge1=1,ncount1=0;
            for(int i=0;i<n;i++)
            {
                int judge2=1;
                for(int j=0;j<all[i].num&&judge2;j++)
                if(all[k].item[t]==all[i].item[j])
                {
                    judge2=0;
                    ncount1++;
                }
            }
            if(ncount1==n)
            {
                for(int p=ncount-1;p>=0&&judge1;p--)
                if(shop[p]==all[k].item[t])
                judge1=0;
                if(judge1)
                shop[ncount++]=all[k].item[t];
            }
        }
    int temp;
    for(int i=0;i<ncount;i++)
        for(int j=0;j<ncount-1;j++)
        {
            if(shop[j]>shop[j+1])
            {
                temp=shop[j];
                shop[j]=shop[j+1];
                shop[j+1]=temp;
            }
        }
    for(int i=0;i<ncount;i++)
    printf("%d ",shop[i]);
    if(ncount==0)printf("NO");
    return 0;
}