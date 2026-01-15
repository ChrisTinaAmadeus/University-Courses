#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int n, maze[20][20], f[20][20];
const int xmove[] = {1, 0, -1, 0};
const int ymove[] = {0, 1, 0, -1};
int minncount = 100000000;
bool inside(int m)
{
    if (m >= 1 && m <= n)
        return true;
    return false;
}
int ncount;
void findminncount(int x, int y)
{
    if (ncount > minncount || maze[x][y] == 0)
        return;
    if (x == n && y == n)
    {
        minncount = min(minncount, ncount);
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        for (int j = 1; j <= maze[x][y]; j++)
        {
            if (inside(x + j * xmove[i]) && inside(y + j * ymove[i]) && f[x + j * xmove[i]][y + j * ymove[i]] != 1)
            {
                f[x + j * xmove[i]][y + j * ymove[i]] = 1;
                ncount++;
                findminncount(x + j * xmove[i], y + j * ymove[i]);
                f[x + j * xmove[i]][y + j * ymove[i]] = 0;
                ncount--;
            }
        }
    }
}
int main()
{
    scanf("%d", &n);
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= n; j++)
        {
            scanf("%d", &maze[i][j]);
        }
    findminncount(1,1);
    printf("%d", minncount);
    return 0;
}