#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char picturecpy[5][60][60];
void print(char a[][60][60], int n, int k)
{
    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= n; j++)
            printf("%c", a[k][i][j]);
        printf("\n");
    }
    printf("\n");
}
int main()
{
    int n, m;
    scanf("%d%d", &n, &m);
    char picture[60][60] = {};
    getchar();
    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= n; j++)
            scanf("%c", &picture[i][j]);
        getchar();
    }
    // printf("\n");
    for (int i = 1; i <= n; i++) // A
    {
        for (int j = n / 2; j >= 1; j--)
        {
            if (n % 2 == 0)
            {
                picturecpy[1][i][n - j + 1] = picture[i][j];
                picturecpy[1][i][j] = picture[i][n - j + 1];
            }
            else
            {
                picturecpy[1][i][n - j + 1] = picture[i][j];
                picturecpy[1][i][j] = picture[i][n - j + 1];
                picturecpy[1][i][n / 2 + 1] = picture[i][n / 2 + 1];
            }
        }
    }
    // print(picturecpy, n, 1);
    for (int i = 1; i <= n; i++) // B
    {
        for (int j = n / 2; j >= 1; j--)
        {
            if (n % 2 == 0)
            {
                picturecpy[2][n - j + 1][i] = picture[j][i];
                picturecpy[2][j][i] = picture[n - j + 1][i];
            }
            else
            {
                picturecpy[2][n - j + 1][i] = picture[j][i];
                picturecpy[2][j][i] = picture[n - j + 1][i];
                picturecpy[2][n / 2 + 1][i] = picture[n / 2 + 1][i];
            }
        }
    }
    // print(picturecpy, n, 2);
    for (int i = 1; i <= n; i++) // D
    {
        for (int j = i; j >= 1; j--)
        {
            picturecpy[4][i][j] = picture[j][i];
            picturecpy[4][j][i] = picture[i][j];
        }
    }
    // print(picturecpy, n, 4);
    for (int i = 1; i <= n; i++) // C
    {
        for (int j = n; j >= 1; j--)
        {
            picturecpy[3][i][j] = picture[n - j + 1][n - i + 1];
            picturecpy[3][n - j + 1][n - i + 1] = picture[i][j];
        }
    }
    // print(picturecpy, n, 3);
    int judge[5] = {};
    for (int i = 1; i <= 4; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            if (strcpy(picture[j], picturecpy[i][j]) != 0)
                judge[i] = 1;
        }
    }
    for (int i = 1; i <= 4; i++)
    {
        if (judge[i] == 0)
            continue;
        for (int j = 1; j <= 4; j++)
        {
            if (j <= i || judge[j] == 0)
                continue;
            bool judgecpy = false;
            for (int k = 1; k <= n; k++)
            {
                for (int l = 1; l <= n; l++)
                {
                    if (picturecpy[i][k][l] != picturecpy[j][k][l])
                    {
                        judgecpy = true;
                        break;
                    }
                }
            }
            if (!judgecpy)
                judge[j] = 0;
        }
    }
    if (m == 1 && n != 1)
    {
        int ncount = 0;
        for (int i = 1; i <= 4; i++)
            if (judge[i] == 1)
                ncount++;
        printf("%d\n", ncount);
        for (int i = 1; i <= n && judge[1] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[1][i][j]);
            }
            printf("\n");
        }
        if (judge[1] == 1)
            printf("\n");
        for (int i = 1; i <= n && judge[2] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[2][i][j]);
            }
            printf("\n");
        }
        if (judge[2] == 1)
            printf("\n");
        for (int i = 1; i <= n && judge[3] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[3][i][j]);
            }
            printf("\n");
        }
        if (judge[3] == 1)
            printf("\n");
        for (int i = 1; i <= n && judge[4] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[4][i][j]);
            }
            printf("\n");
        }
    }
    else if (m == 0 && n != 1)
    {
        int ncount = 0;
        for (int i = 1; i <= 2; i++)
            if (judge[i] == 1)
                ncount++;
        printf("%d\n", ncount);
        for (int i = 1; i <= n && judge[1] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[1][i][j]);
            }
            printf("\n");
        }
        if (judge[1] == 1)
            printf("\n");
        for (int i = 1; i <= n && judge[2] == 1; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                printf("%c", picturecpy[2][i][j]);
            }
            printf("\n");
        }
    }
    else if (n == 1)
    {
        printf("1\n");
        printf("%c", picture[n][n]);
    }
    return 0;
}