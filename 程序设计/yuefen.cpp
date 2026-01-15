#include<stdio.h>
#include<iostream>
using namespace std;
int maxyue(int a,int b)
{
    int k;
    for(int i=min(a,b);i>0;i--)
    {
        if(a%i==0&&b%i==0)
        {
            k=i;
            break;
        }
    }
    return k;
}
int main()
{
    int zi,mu;
    scanf("%d/%d",&zi,&mu);
    int zi1,mu1;
    zi1=zi/maxyue(zi,mu);
    mu1=mu/maxyue(zi,mu);
    printf("%d/%d",zi1,mu1);
    return 0;
}