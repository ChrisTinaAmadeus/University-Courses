#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int power(int a)
{
    int sum=1;
    for(int i=a;i>0;i--)
    sum*=10;
    return sum;
}
int main() 
{
    int a,b;
    scanf("%d%d",&a,&b);
    long long sum=0;
    for(int i=a;i<=b;i++)
    {
        int j=1;
        for(int k=i;k>0;)
        {
            if(k/10!=0)j++;
            k/=10;
        }
        int final=(i*i)%power(j);
        if(final==i)sum+=i;
    }
    printf("%d",sum);
    return 0;
}