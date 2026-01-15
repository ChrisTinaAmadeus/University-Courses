#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int s[3][3];
char final[1000][1000];
void natural(int t, int r, int c)
{
    for (int i = 0; i < t; i++)
        for (int j = 0; j < t; j++)
        {
            final[r + i][c + j] = ' ';
        }
}
void pattern(int n, int r, int c, char d)
{
    if (n == 1)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                if (s[i][j] == 1)
                    final[r + i][c + j] = d;
                else
                    final[r + i][c + j] = ' ';
            }
    }
    else
    {
        int t;
        if (n == 2)
            t = 3;
        else
            t = 9;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                if (s[i][j] == 1)
                {
                    pattern(n - 1, r + i * t, c + j * t, d);
                }
                else
                    natural(t, r + i * t, c + j * t);
            }
    }
}
int main()
{
    char dot;
    int n;
    scanf("%c %d", &dot, &n);
    int t;
    if (n == 1)
        t = 3;
    if (n == 2)
        t = 9;
    if (n == 3)
        t = 27;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
        {
            scanf("%d", &s[i][j]);
        }
    pattern(n, 0, 0, dot);
    for (int i = 0; i < t; i++)
    {
        for (int j = 0; j < t; j++)
        {
            printf("%c", final[i][j]);
        }
        printf("\n");
    }
    return 0;
}