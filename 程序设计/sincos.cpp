#include<stdio.h>
#include<math.h>
#include<iostream>
using namespace std;
int main()
{
    double x,m,l,s=0.0,c=0.0;
    double t;//power要用double的形式
    scanf("%lf%lf",&x,&m);
    double y,π;
    π=3.1415926;
    if(x>2*π)//大数的指数运算会爆掉，减2π
    {
    for(x;x>-2*π;x-=2*π)//接近2π的数会爆掉，所以取0-2π=-2π
    y=x;
    }
    else y=x;
    long long a,n,i,p,q=1;
    for(i=1;i<=33;i+=2)
    {
        n=(i+1)/2;
        a=pow(-1,n-1);
        t=pow(double(y),i);
        q=1;//每次规定q的值来计算阶乘，否则会爆
            for(p=i;p>=1;p--)
            {
                q=p*q;
            }
        s+=t*a/q;
        if(fabs(s-sin(y))<m)//与标准函数进行比较，fabs为绝对值运算，小于误差m则结束循环
        break;
    }
    for(i=1;i<=33;i+=2)
    {
        n=(i+1)/2;
        a=pow(-1,n-1);
        t=pow(double(y),i-1);
        q=1;
        if(i==1)//一定要注意写两个等号
        q=1;
        else if(i>1)
        {
            for(p=i-1;p>=1;p--)
            {
                q=p*q;
            }
        }
        c+=t*a/q;
        if(fabs(c-cos(y))<m)
        break;
    }
    printf("%lf\n",s);
    printf("%lf\n",c);
    return 0;
}