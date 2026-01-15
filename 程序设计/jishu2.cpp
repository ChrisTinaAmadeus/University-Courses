#include<stdio.h>
#include<iostream>
#include<cmath>
using namespace std;
int main()
{
    int a,b,n,k,m,p,q,t;
    scanf("%d%d%d%d%d",&a,&b,&n,&k,&m);//读入需要的数
    p=pow(10,m-1);
    q=pow(10,m);
    t=a/10;
    int i,x=0;
    for(i=10*t+n;i<=b;i+=10)//保证个位数是n，同时减少循环次数
    {
        if(i>=a&&i%k==0&&i>=p&&i<q)
        x++;
    }
    printf("%d",x);
    return 0;
}