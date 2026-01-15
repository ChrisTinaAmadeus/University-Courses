#include<stdio.h>
int main()
{
    long long a;
    scanf("%lld",&a);
    printf("%15lld\n",a);
    printf("%15u\n",a);
    printf("%15o\n",a);
    printf("%15x\n",a);
    return 0;
}