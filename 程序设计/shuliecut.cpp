#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    int N;
    long long M;
    scanf("%d%lld", &N, &M);
    long long sum = 0;
    int ncount = 0;
    for (int i = 1; i <= N; i++)
    {
        long long a;
        scanf("%lld", &a);
        sum += a;
        if(sum>M)
        {
            sum=a;
            ncount++;
        }
    }
    printf("%lld", ncount + 1);
    return 0;
}