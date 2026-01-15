#include<stdio.h>
#include<string.h>
#include<math.h>
#include<iostream>
using namespace std;
int main()
{
    float n,a,b,c;
    scanf("%f%f%f%f",&n,&a,&b,&c);
    int m=n/300;
    float sum=n-40*m;
    if(sum>=a)sum-=b;
    sum-=c;
    if(sum<0)printf("0.00");
    else printf("%.2f",sum);
    return 0;
}