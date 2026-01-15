#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char map[110][110];
int f[110][110];
int n, m;
int ncount,maxS,S1;
const int nchange[] = {-1, 0, 1};
const int mchange[] = {-1, 0, 1};
bool inside(int a,int maxline)
{
    if(a>=0&&a<=maxline)return true;
    else
        return false;
}
void findmaxisland(int x,int y)
{
    for (int i = 0; i <= 2;i++)
        for (int j = 0; j <= 2;j++)
        {   
            if(map[x+nchange[i]][y+mchange[j]]=='I')
            if(f[x+nchange[i]][y+mchange[j]]!=1&&inside(x+nchange[i],n-1)&&inside(y+mchange[j],m-1))
            {
                
                f[x + nchange[i]][y + mchange[j]] = 1;
                S1 += 1;
                findmaxisland(x + nchange[i], y + mchange[j]);
            }
        }
}
int main()
{
    scanf("%d%d", &n, &m);
    for (int i = 0; i < n; i++)
        scanf("%s", map[i]);
    for (int i = 0; i < n;i++)
    {
        for (int j = 0; j < m;j++)
        {
            if(map[i][j]=='I'&&f[i][j]!=1)
            {
                f[i][j] = 1;
                S1 = 1;
                findmaxisland(i, j);
                maxS = max(S1, maxS);
                S1 = 0;
                ncount += 1;
            }
        }
    }
    printf("%d %d", ncount, maxS);
    return 0;
}