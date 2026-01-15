#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
long long weishu(long long n)
{
    long long sum=0;
    while(n>0)
    {
        n/=10;
        sum++;
    }
    return sum;
}
long long ncount,N;
void feibonacci(long long a1,long long a2)
{
    if(a1+a2>=N)
    return;
    long long a=a1+a2;
    long long cpya=a;
    long long wei=weishu(a),he=0;
    for(long long i=1;i<=wei;i++)
    {
        he+=(cpya%10);
        cpya/=10;
    }
    double result=(he*1.0)/(wei*1.0);
    if(result>4.0)
    ncount++;
    feibonacci(a2,a);
}
int main()
{
    scanf("%lld",&N);
    feibonacci(0,1);
    printf("%lld",ncount);
    return 0;
}