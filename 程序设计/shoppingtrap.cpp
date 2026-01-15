#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    long long n;
    scanf("%lld",&n);
    struct all
    {
        long long order;
        double a,b;
    };
    all shop[100000]={};
    struct trap
    {
        long long order;
        double c;
    };
    trap traps[100000]={};
    for(long long i=0;i<n;i++)
    {
        scanf("%lld%lf%lf",&shop[i].order,&shop[i].a,&shop[i].b);
    }
    long long j=0;
    for(long long i=0;i<n;i++)
    {
        if(shop[i].a<=shop[i].b)
        {
            traps[j].order=shop[i].order;
            traps[j].c=shop[i].b-shop[i].a;
            j++;
        }
    }
    for(long long i=0;i<j;i++)
    {
        printf("%d %.2lf\n",traps[i].order,traps[i].c);
    }
    float baifenbi;
    baifenbi=(j/(n*1.0))*100;
    printf("%.2f%%",baifenbi);
    return 0;
}