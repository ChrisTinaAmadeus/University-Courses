#include<stdio.h>
int main()
{
    long long a;
    scanf("%lld",&a);
    long long i;
    for(i=1;i<=a;i++)
    {
        if(a%i==0)
        printf("%lld ",i);
    }
    return 0;
}