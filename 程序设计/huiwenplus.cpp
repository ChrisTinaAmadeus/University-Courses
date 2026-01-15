#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
long long power(int a)
{
    long long sum=1;
    int i=a;
    while(i>0)
    {
        sum*=10;
        i--;
    }
    return sum;
}
long long ni(long long a)
{
    int wei[20]={},i=0;
    while(a>0)
    {
        wei[i]=a%10;
        a=a/10;
        i++;
    }
    int zong=0;
    for(int j=19;j>=0;j--)
    if(wei[j]!=0)
    {
        zong=j+1;
        break;
    }
    long long sum=0;
    int k=0;
    for(int j=zong-1;j>=0;j--)
    {
        long long WEI;
        WEI=power(j);
        sum+=(WEI*wei[k]);
        k++;
    }
    return sum;
}
long long hui(long long a)
{
    long long sum=a;
    if(a<10) return a;
    for(int i=0;;i++)
    {
        long long he=sum;
        if(he==ni(he)) return sum;
        sum+=ni(he);   
    }
}
long long time(long long a)
{
    long long sum=a;
    if(a<10) return 0;
    for(long long P=0;;P++)
    {
        long long he=sum;
        if(he==ni(he)) return P;
        sum+=ni(he);   
    }
}
int main()
{
    long long num[10]={};
    for(int i=0;i<10;i++)
        scanf("%lld",&num[i]);
    typedef struct 
    {
        long long initial;
        long long times;
        long long final;
    }huiwen;
    huiwen shu[10]={};
    for(int i=0;i<10;i++)
    {
        shu[i].initial=num[i];
        shu[i].times=time(num[i]);
        shu[i].final=hui(num[i]);
    }
    huiwen temp;
    for(int i=0;i<10;i++)
    {
        for(int j=0;j<9;j++)
        {
            if(shu[j].times<shu[j+1].times)
            {
                temp=shu[j];
                shu[j]=shu[j+1];
                shu[j+1]=temp;
            }
        }
    }
    for(int i=0;i<10;i++)
    {
        for(int j=0;j<9;j++)
        {
            if(shu[j].times==shu[j+1].times&&shu[j].final>shu[j+1].final)
            {
                temp=shu[j];
                shu[j]=shu[j+1];
                shu[j+1]=temp;
            }
        }
    }
    for(int i=0;i<10;i++)
    {
        for(int j=0;j<9;j++)
        {
            if(shu[j].times==shu[j+1].times&&shu[j].final==shu[j+1].final&&shu[j].initial>shu[j+1].initial)
            {
                temp=shu[j];
                shu[j]=shu[j+1];
                shu[j+1]=temp;
            }
        }
    }
    for(int i=0;i<10;i++)
    printf("%lld %lld %lld\n",shu[i].initial,shu[i].times,shu[i].final);
    return 0;
}