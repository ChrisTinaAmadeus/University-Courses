#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int allnum[1000] = {};
int main()
{    
    int n;
    scanf("%d", &n);
    int num[] = {6, 2, 5, 5, 4, 5, 6, 3, 7, 6};
    for (int i = 0; i < 1000; i++)
    {
        if (i < 10)
            allnum[i] = num[i];
        else
        {
            int I = i;
            while (I > 0)
            {
                int a = I % 10;
                I /= 10;
                allnum[i] += num[a];
            }
            if (allnum[i] > 20)
                allnum[i] = -1;
        }
    }
    n -= 4;
    long long sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        if (allnum[i] < 0)
            continue;
        for (int j = 0; j < 1000; j++)
        {
            if(i+j>=1000)continue;
            if (allnum[j] < 0)
                continue;
            if (allnum[i + j] < 0)
                continue;
            if (allnum[i] + allnum[j] + allnum[i + j] != n)
                continue;
            //printf("%d + %d = %d\n",i,j,i+j);
            sum += 1;
        }
    }
    printf("%lld", sum);
    return 0;
}