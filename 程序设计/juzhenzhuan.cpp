#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    int n, m;
    scanf("%d%d", &n, &m);
    int juzhen[60][60] = {};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
        {
            scanf("%d", &juzhen[i][j]);
        }
    int order;
    scanf("%d", &order);
    if (order >= 0)
    {
        if ((order / 90) % 4 == 1)
        {
            for (int i = 0; i < m; i++)
            {
                for (int j = n - 1; j >= 0; j--)
                {
                    printf("%d ", juzhen[j][i]);
                }
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 2)
        {
            for (int i = n - 1; i >= 0; i--)
            {
                for (int j = m - 1; j >= 0; j--)
                    printf("%d ", juzhen[i][j]);
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 3)
        {
            for (int i = m-1; i >=0; i--)
            {
                for (int j = 0; j<n; j++)
                    printf("%d ", juzhen[j][i]);
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 0)
        {
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < m; j++)
                    printf("%d ", juzhen[i][j]);
                printf("\n");
            }
        }
    }
    else
    {
        order *= (-1);
        if ((order / 90) % 4 == 1)
        {
            for (int i = m - 1; i >= 0; i--)
            {
                for (int j = 0; j < n; j++)
                    printf("%d ", juzhen[j][i]);
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 2)
        {
            for (int i = n - 1; i >= 0; i--)
            {
                for (int j = m - 1; j >= 0; j--)
                    printf("%d ", juzhen[i][j]);
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 3)
        {
            for (int i = 0; i < m; i++)
            {
                for (int j = n - 1; j >= 0; j--)
                    printf("%d ", juzhen[j][i]);
                printf("\n");
            }
        }
        if ((order / 90) % 4 == 0)
        {
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < m; j++)
                    printf("%d ", juzhen[i][j]);
                printf("\n");
            }
        }
    }
    return 0;
}