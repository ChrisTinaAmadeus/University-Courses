#include<stdio.h>
#include<string.h>
#include<math.h>
#include <stdbool.h>
#include<iostream>
using namespace std;
int power(int a)
{
    return a*a;
}
bool isPrime(int num) {
    if (num <= 1) return false;
    for (int i = 2; i <= sqrt(num); i++) {
        if (num % i == 0) return false;
    }
    return true;
}
int main()
{
    int sushu[5000]={},m=0;
    for(int i=2;i<5000;i++)
    {
        int k=0;
        for(int j=i-1;j>=2;j--)
        {
        if(i%j==0)
        k=1;
        }
        if(!k)
        {
            sushu[m]=i;
            m++;
        }
    }
    int N;
    scanf("%d",&N);
    int I,J,MIN=1000000;
    for(int i=0;i<m-1;i++)
        for(int j=0;j<m-1;j++)
        {
            if(sushu[i]*sushu[j]>N&&sushu[i]*sushu[j]<MIN)
            {
                I=i;
                J=j;
                MIN=sushu[i]*sushu[j];
            }
        }
    printf("%d %d\n",sushu[I],sushu[J]);
    int powerall[150]={};
    for(int i=1;i<=130;i++)
    powerall[i]=power(i);
    int I1,J1,K,L,temp=0;
    for(int i=0;i<130&&(!temp);i++)//《暴力枚举才是每道题的终点》
        for(int j=0;j<130&&(!temp);j++)
            for(int k=0;k<130&&(!temp);k++)
                for(int l=0;l<130&&(!temp);l++)
                {
                    if(powerall[i]+powerall[j]+powerall[k]+powerall[l]==MIN)
                    {
                        I1=(i!=0);
                        J1=(j!=0);
                        K=(k!=0);
                        L=(l!=0);
                        temp=1;
                    }
                }
    printf("%d",I1+J1+K+L);
    return 0;
}