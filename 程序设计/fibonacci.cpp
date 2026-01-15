#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
void juzhen(long long n,long long k,int a11,int a12,int a21,int a22)
{
    if(n==0||n==1)printf("%lld",n);
    else 
    {
        while(k+15<=n)
        {
            int A11=a11,A12=a12,A21=a21,A22=a22;
            a11=(A11*987+A12*610)%10000;
            a12=(A11*610+A12*377)%10000;
            a21=(A21*987+A22*610)%10000;
            a22=(A21*610+A22*377)%10000;
            k+=15;
        }
        while(k<n)
        {
            int A11=a11,A12=a12,A21=a21,A22=a22;
            a11=(A11+A12)%10000;
            a12=A11%10000;
            a21=(A21+A22)%10000;
            a22=A21%10000;
            k++;
        }
        printf("%d",a12);
    }
}
int main() 
{
    long long N;
    scanf("%lld",&N);
    juzhen(N,1,1,1,1,0);
    return 0;
}