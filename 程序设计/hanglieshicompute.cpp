#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
long long sum=0;
int n;
int flag[20],a[20],b[20];
void cheng(int k,int num[][20])
{
    int ni=0;
    long long part=1;
    if(k==n)
    {
        for(int i=0;i<n;i++)
        {
            part*=a[i];
            if(i<n-1)
            {
                for(int j=i+1;j<n;j++)
                if(b[i]>b[j]) ni++;
            }
        }
        if(ni%2==0)
        sum+=part;
        else sum-=part;
    }
    for(int i=0;i<n;i++)//列指标，每行里选一个数
    {
        if(flag[i]==0)
        {
            a[k]=num[k][i];
            b[k]=i;
            flag[i]=1;
            cheng(k+1,num);
            flag[i]=0;
        }
    }
}
int main() 
{
    int num[20][20]={};
    scanf("%d",&n);
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            scanf("%d",&num[i][j]);
    cheng(0,num);
    printf("%lld",sum);
    return 0;
}