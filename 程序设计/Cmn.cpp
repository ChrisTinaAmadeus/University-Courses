#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
long long cheng(int n,int a)
{
    long long sum=1;
    int ncount=1;
    while(ncount<=a)
    {
        sum*=n;
        n--;
        ncount++;
    }
    return sum;
}
int main() 
{
    int n;
    scanf("%d",&n);
    long long up=cheng(n*n,n),down=cheng(n,n);
    printf("%lld",up/down);
    return 0;
}