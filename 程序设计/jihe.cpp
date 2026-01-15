#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
long long s[1000000]={};//关键：开全局变量，这样才不会超出栈的范围
int num[2000001]={};
int main()
{
    int n;
    int mi=20000000,ma=0;
    scanf("%d",&n);
    for(int i=0;i<n;i++)
    {
        int m;
        scanf("%d",&m);
        for(int j=0;j<m;j++)
        {
            scanf("%lld",&s[j]);
            int k=s[j];
            num[k+1000000]++;
            mi=min(mi,k+1000000);
            ma=max(ma,k+1000000);
        }
    }
    long long sum=0;
    for(int i=mi;i<=ma;i++)
    {
        if(num[i]==1) sum+=(i-1000000);
    }
    printf("%lld",sum);
    return 0;
}