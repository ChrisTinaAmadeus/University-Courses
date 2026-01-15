#include <stdio.h>//yoj-643
#include <string.h>
#include<iostream>
using namespace std;
float way1(int a1,int a2,int b1,int b2)
{
    float sum=(a1*b1+a2*b2)*1.0;
    if(b1+b2==2) sum*=0.9;
    if(b1+b2==3||b1+b2==4) sum*=0.8;
    if(b1+b2==5) sum*=0.7;
    return sum;
}
int way2(int a1,int a2,int b1,int b2)
{
    int sum=a1*b1+a2*b2;
    int s=sum,n=0;
    while(s>=500)
    {
        n++;
        s-=500;
    }
    sum-=n*100;
    return sum;
}
int main() 
{
    int price1,price2,num1,num2;
    scanf("%d%d%d%d",&price1,&price2,&num1,&num2);
    if(way1(price1,price2,num1,num2)<=way2(price1,price2,num1,num2))
    printf("1 %.1f",way1(price1,price2,num1,num2)*1.0);
    else printf("2 %.1f",way2(price1,price2,num1,num2)*1.0);
    return 0;
}