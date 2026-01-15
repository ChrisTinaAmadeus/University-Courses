#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    int n;
    scanf("%d", &n);
    int num[200] = {};
    for (int i = 0; i < n;i++)
        scanf("%d", &num[i]);
    int maxsum = -10000000;
    for (int i = 0; i < n;i++)
    {
        int sum = 0;
        for (int j = i; j < n;j++)
        {
            sum += num[j];
            maxsum = max(sum, maxsum);
        }
    }
    printf("%d", maxsum);
    return 0;
}