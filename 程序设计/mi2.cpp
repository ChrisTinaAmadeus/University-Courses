#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int power(int a)
{
    int sum=1;
    for(a;a>0;a--)
    sum*=2;
    return sum;
}
void break2(int a)
{
    int mi=0;
    for(mi;;mi++)
    {
        if(a-power(mi+1)<0)
        break;
    }
    if(a!=power(mi))
    {
        if(mi!=1)
        {
            printf("2(");
            break2(mi);
            printf(")+");
        }
        if(mi==1)
        {
            printf("2+");
        }
    }
    if(a==power(mi)) 
    {
        if(mi!=0&&mi!=1)
        {
        printf("2(");
        break2(mi);
        printf(")");
        }
        if(mi==1)
        {
            printf("2");
            return;
        }
        if(mi==0)
        {
            printf("2(0)");
            return;
        }
    }
    a-=power(mi);
    if(a!=0)
    break2(a);
}
int main() 
{
    int n;
    scanf("%d",&n);
    break2(n);
    return 0;
}