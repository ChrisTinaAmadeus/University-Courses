#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#define MAX 60
using namespace std;
int board[MAX][MAX];
int f[MAX][MAX];
int way1[MAX + 10], way2[MAX + 10];
int l = 1, length = 0, minlength = 100000, minl = 0;
int n;
bool judge = false;
void findway(int now, int des)
{
    if (now == des)
    {
        judge = true;
        if (minlength > length)
        {
            minl = 0;
            for (int i = 0; i < l; i++)
            {
                way2[i] = way1[i];
                minl++;
            }
        }
        minlength =min(minlength,length);
        return;
    }
    if (length >= minlength)
        return;
    if (board[now][des] != -1)
    {
        way1[l] = des;
        length += board[now][des];
        l++;
        findway(des, des);
        l--;
        way1[l] = 0;
        length -= board[now][des];
    }
    for (int i = 0; i < n; i++)
    {
        if (board[now][i] != -1 && f[now][i] != 1)
        {
            f[now][i] = 1;
            way1[l] = i;
            length += board[now][i];
            l++;
            findway(i, des);
            l--;
            way1[l] = 0;
            length -= board[now][i];
            f[now][i] = 0;
        }
    }
}
int main()
{
    scanf("%d", &n);
    int s, d;
    scanf("%d%d", &s, &d);
    way1[0] = s;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            scanf("%d", &board[i][j]);
    findway(s, d);
    if (judge)
    {
        for (int i = 0; i < minl - 1; i++)
            printf("%d->", way2[i]);
        printf("%d", d);
    }
    else
        printf("-1");
    return 0;
}