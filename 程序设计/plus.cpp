#include<stdio.h>
#include<string.h>//本题关键在于使用long long避免数据爆掉
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
using namespace std;
long long power(int a)//计算10的a次方
{
    long long sum=1;
    for(int i=1;i<=a;i++)
    sum*=10;
    return sum;
}
long long num(int a,int b,long long c[])//将数组中数字组成一个整体的数，1,2,3→123
{
    long long d=0,sum=0;
    for(int i=b-1;i>=a;i--)
    {
        long long j=power(d)*c[i];
        sum+=j;
        d++;
    }
    int k=0;
    for(int i=b-1;i>=a;i--)
    {
        if(c[i]==0) k++;
    }
    if(k==b-a&&k>1) sum=1;
    return sum;
}
int main()
{
    char S[15]={};
    gets(S);
    long long s[15]={};
    for(int i=0;i<strlen(S);i++)
    {
        s[i]=S[i]-'0';
    }
    long long plus[1000]={},p=0;
    for(int i=1;i<=strlen(S)-3;i++)
        for(int j=i+1;j<=strlen(S)-2;j++)//j=i+1避免重复，避免加号位置错乱
            for(int k=j+1;k<=strlen(S)-1;k++)//同理
            {
                if((num(0,i,s)!=0&&s[0]==0)||(num(i,j,s)!=0&&s[i]==0)||(num(j,k,s)!=0&&s[j]==0)||(num(k,strlen(S),s)!=0&&s[k]==0))
                continue;//排除诸如02，043的情况
                if(i!=j&&i!=k&&j!=k)
                {
                    
                    plus[p]=num(0,i,s)+num(i,j,s)+num(j,k,s)+num(k,strlen(S),s);
                    p++;
                }
            }
    int P=0;
    long long temp;
    for(int i=0;;i++)
    {
        if(plus[i]!=0)P++;
        else break;
    }
    for(int i=0;i<P;i++)
    {
        for(int j=0;j<P-1;j++)
        {
            if(plus[j]<plus[j+1])
            {
                temp=plus[j+1];
                plus[j+1]=plus[j];
                plus[j]=temp;
            }
        }
    }
    for(int i=0;i<P;i++)
    printf("%lld ",plus[i]);
    return 0;
}