#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
char board[100][100];
int f[100][100];
int w, h;
struct orderall
{
    int good;
    int way;
};
struct data1
{
    int wei[10];
    int ncount = 10000;
};
data1 all[1000];
const int xx[] = {0, -1, 1, 0};
const int yy[] = {1, 0, 0, -1};

bool inside(int x, int n)
{
    if (x >= 0 && x <= n + 1)
        return true;
    else
        return false;
}
int mcount = 1;
void find1(int k, int type, int r, int c) // 通过type判断是否为新线段
{
    if (all[k].wei[3] == c && all[k].wei[4] == r)
    {
        all[k].ncount = min(all[k].ncount, mcount);
        return;
    }
    if (mcount > 10 || mcount > all[k].ncount)//如果mcount更大了说明走复杂了，直接结束，省时间
        return;
    for (int i = 0; i <= 3; i++)
    {
        if (inside(r + xx[i], h) && inside(c + yy[i], w) && f[r + xx[i]][c + yy[i]] != 1)
        {
            if (board[r + xx[i]][c + yy[i]] == 'X' && r + xx[i] == all[k].wei[4] && c + yy[i] == all[k].wei[3])
            {
                f[r][c]=1;
                if(i!=type&&type!=4) mcount++;
                find1(k,i,r+xx[i],c+yy[i]);
                if(i!=type&&type!=4)mcount--;
                f[r][c]=0;
            }
            else if (board[r + xx[i]][c + yy[i]] != 'X')
            {
                f[r][c]=1;
                if(i!=type&&type!=4) mcount++;
                find1(k,i,r+xx[i],c+yy[i]);
                if(i!=type&&type!=4)mcount--;
                f[r][c]=0;
            }
        }
    }
    return;
}
int main()
{
    scanf("%d%d", &w, &h);
    getchar();
    for (int i = 1; i <= h; i++)
    {
        for (int j = 1; j <= w; j++)
        {
            scanf("%c", &board[i][j]);
            if (board[i][j] == ' ')
                board[i][j] = 0;
        }
        getchar();
    }
    int n;
    scanf("%d", &n);
    for (int i = 0; i < n; i++)
    {
        for (int j = 1; j <= 4; j++)
            scanf("%d", &all[i].wei[j]); // 注意是先列后行
    }
    for (int k = 0; k < n; k++)
    {
        int mcount = 1;
        f[all[k].wei[2]][all[k].wei[1]] = 1;
        find1(k, 4, all[k].wei[2], all[k].wei[1]);
        if (all[k].ncount > 10)
            printf("impossible\n");
        else
            printf("%d\n", all[k].ncount);
    }
    return 0;
}