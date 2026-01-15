#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int f[15],ncount,judge;
int plus1(int a[],int s,int n)
{
    int judge1=0;
    int i=1;
    int acpy[15]={};
    for(int j=0;j<n;j++)
    {
        acpy[j]=a[j];
    }
    int k=0;
    while(i<=n-1)
    {
        k=0;
        for(int j=0;j<n-i;j++)
        {
            acpy[k++]=acpy[j]+acpy[j+1];
        }
        i++;
    }
    if(acpy[0]==s)judge1=1;
    return judge1;
}
void shulie(int k,int n,int s,int a[])
{
    if(judge)return;
    if(k==n+1)
    {
        if(plus1(a,s,n))
        {
            judge=1;
            for(int i=0;i<n;i++)
            printf("%d ",a[i]);
            return;
        }
        else return; 
    }
    else
    {
        for(int i=1;i<=n;i++)
        {
            if(f[i]!=1)
            {
                f[i]=1;
                a[ncount]=i;
                ncount++;
                shulie(k+1,n,s,a);
                f[i]=0;
                ncount--;
                a[ncount]=0;
            }
        }
    }
}
int main() 
{
    int n,sum;
    scanf("%d%d",&n,&sum);
    int all[15]={};
    shulie(1,n,sum,all);
    return 0;
}