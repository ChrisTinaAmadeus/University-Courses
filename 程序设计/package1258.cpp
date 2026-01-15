#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
long long record[300]={0,0,1};
long long dfs(int n)
{
    if(n==1)
        return 0;
    else if(n==2)
        return 1;
    else if(n>=3)
    {
        if(record[n-1]==0)
            record[n - 1] = dfs(n - 1);
        if(record[n-2]==0)
            record[n - 2] = dfs(n - 2);
        return (record[n - 1] + record[n - 2]) * (n - 1) % 202420242024;
    }
    return 0;
}
int main()
{
    int n;
    scanf("%d", &n);
    printf("%lld", dfs(n));
    return 0;
}