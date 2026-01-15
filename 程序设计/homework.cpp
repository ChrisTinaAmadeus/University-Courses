#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int main()
{
    int n;
    scanf("%d", &n);
    int N = 0;
    int dif[100100] = {};
    for (int i = 0; i < n; i++)
    {
        int a;
        scanf("%d", &a);
        dif[a]++;
        N=max(N,dif[a]);
    }
    printf("%d", N);
    return 0;
}