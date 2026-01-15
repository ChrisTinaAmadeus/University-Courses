#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char picture[500][500];
int standard[5][5];
char dot;
void setpicture()
{
    for (int i = 0; i < 500; i++)
        for (int j = 0; j < 500; j++)
            picture[i][j] = ' ';
}
/*
    1  1
      1
    1  1
*/
int power(int n, int m)
{
    int sum = 1;
    while (m > 0)
    {
        sum *= n;
        m -= 1;
    }
    return sum;
}
void dfs(int n, int m, int r, int c)
{
    if (n > 1)
    {
        for (int i = 0; i < m; i++)
            for (int j = 0; j < m; j++)
            {
                if (standard[i][j] != 0)
                    dfs(n - 1, m, r + i *power(m,n-1), c + j *power( m,n-1));
            }
    }
    if (n == 1)
    {
        for (int i = 0; i < m; i++)
            for (int j = 0; j < m; j++)
            {
                if (standard[i][j] != 0)
                    picture[r + i][c + j] = dot;
            }
    }
    return;
}

int main()
{
    setpicture();
    scanf("%c", &dot);
    int m;
    scanf("%d", &m);
    for (int i = 0; i < m; i++)
        for (int j = 0; j < m; j++)
            scanf("%d", &standard[i][j]);
    int n;
    scanf("%d", &n);
    dfs(n, m, 0, 0);
    for (int i = 0; i < power(m, n); i++)
    {
        for (int j = 0; j < power(m, n); j++)
            printf("%c", picture[i][j]);
        printf("\n");
    }
    return 0;
}