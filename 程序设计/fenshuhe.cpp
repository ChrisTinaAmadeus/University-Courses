#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    long long a1,b1,a2,b2;
    scanf("%lld%lld%lld%lld",&a1,&b1,&a2,&b2);
    long long i;
    for(i=max(b1,b2);;i+=max(b1,b2))
    {
        if(i%b1==0&&i%b2==0)
        break;
    }
    long long c1=i/b1,c2=i/b2;
    a1*=c1;a2*=c2;
    long long a=a1+a2;
    long long j=1;
    int all[10010]={};
    for(long long k=2;k<=10005;k++)
    all[k]=1;
    for(long long k=2;k<=10005;k++)
    {
        if(all[k]!=0)
        {
            if(a%k==0&&i%k==0)
            {
                j=k;
            }
            else
            {
                for(long long t=k;t<10005;t*=k)
                {
                    all[t]=0;
                }
            }
        }
        
    }
    a/=j;
    i/=j;
    printf("%lld/%lld",a,i);
    return 0;
}