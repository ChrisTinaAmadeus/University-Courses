#include<stdio.h>
#include<iostream>
using namespace std;
#include<cmath>
int main()
{
    double x,f,g;
    scanf("%lf",&x);
    if(fabs(x)<=1)
    {
        g=fabs(x-1)-2;
    }
    else
    {
        g=1/(1+x*x);
    }
    if(fabs(g)<=1)
    {
        f=fabs(g-1)-2;
    }
    else
    {
        f=1/(1+g*g);
    }
    printf("%.2lf",f);
    return 0;
}