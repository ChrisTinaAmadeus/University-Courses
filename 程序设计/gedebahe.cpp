#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
struct number
{
    int mami[5];
    int same[5];
    long long ncount;
};
number final[500];
int prime[1000010];
void sushu(int n)
{
    for(int i=0;i<=n;i++)
    prime[i]=1;
    prime[0] = prime[1] = 0;  // 0和1不是质数
    for (int i = 2; i * i <= n; ++i) //开根号即可（数论知识）
    {
        if (prime[i]) {
            for (int j = i * i; j <= n; j += i) {
                prime[j] = 0;  // 标记非质数
            }
        }
    }
}
void maxmin1(int a,int t)
{
    int n=a-1,judge1=1;
    while(n>=(a/2))
    {
        if(prime[n]==1)
        {
            if(prime[a-n]==1)
            {
                if(judge1)
                {
                    final[t].mami[2]=n;
                    final[t].mami[1]=a-n;
                    judge1=0;
                }
                final[t].same[2]=n;
                final[t].same[1]=a-n;
                final[t].ncount++;
            }
        }
        n--;
    }
}
int main()
{
    int n;
    scanf("%d",&n);
    int ma=0;
    int all[500]={};
    for(int i=0;i<n;i++)
    {
        scanf("%d",&all[i]);
        ma=max(ma,all[i]);
    }
    sushu(ma);
    for(int i=0;i<n;i++)
    {
        maxmin1(all[i],i);
        printf("%d+%d %d+%d %lld\n",final[i].mami[1],final[i].mami[2],final[i].same[1],final[i].same[2],final[i].ncount);
    }
    return 0;
}