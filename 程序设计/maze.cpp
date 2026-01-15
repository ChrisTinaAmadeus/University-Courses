#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int judge,flag[15][15];
void find1(int n,int m,int migong[][15],int x,int y,int way)
{
    flag[x][y]=1;
    if(x==n&&y==m)
    {
        printf("YES");
        judge=1;
        return;
    }
    else
    {
        if(y==0) 
        {
            printf("NO");
            judge=1;
        }
        if(way==1&&!judge)//往右走
        {
            if(migong[x][y+1]==1)find1(n,m,migong,x,y+1,1);
            else find1(n,m,migong,x,y,2);
        }
        if(way==2&&!judge)//往下走
        {
            if(migong[x+1][y]==1)find1(n,m,migong,x+1,y,1);
            else 
            {
                if(flag[x-1][y]==1)
                find1(n,m,migong,x,y-1,2);
                else find1(n,m,migong,x-1,y,1);
            }
        }
    }
}
int main() 
{
    int n,m;
    scanf("%d%d",&n,&m);
    int maze[15][15]={};
    for(int i=1;i<=n;i++)
        for(int j=1;j<=m;j++)
        {
            scanf("%d",&maze[i][j]);
        }
    find1(n,m,maze,1,1,1);
    return 0;
}