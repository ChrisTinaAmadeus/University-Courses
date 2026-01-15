#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int prime[2000];
void isprime()
{
    for(int i=0;i<1050;i++)
    prime[i]=1;
    prime[0]=0;
    prime[1]=0;
    for(int i=2;i*i<1050;i++)
    {
        if(prime[i]!=0)
        for(int j=i*i;j<1050;j+=i)
        prime[j]=0;
    }
}
struct num
{
    int ncount;
    int number;
    int accumulate[4000][5];
};
num all[5000]={};
int main() 
{
    int N;
    scanf("%d",&N);
    isprime();
    for(int l=0;l<N;l++)
    {
        scanf("%d",&all[l].number);
    }
    for(int l=0;l<N;l++)
    {
        for(int i=2;i<=all[l].number-4&&i<1000;i++)
        {
            if(!prime[i])continue;
            for(int j=i+1;j<=all[l].number-i-2&&j<1000;j++)
            {
                if(!prime[j])continue;
                int k=all[l].number-i-j;
                if(k>=1000)continue;
                if(prime[k]&&k>j&&k>i)
                {
                    
                    all[l].ncount++;
                }
            }
        }
    printf("%d ",all[l].ncount);
    }
    return 0;
}