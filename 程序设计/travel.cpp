#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int ncount=-10000,ncount1,f[10][10];
void walk(int n,int m,int t[10][10],int x,int y)
{
    if(x==n&&y==m)
    {
        ncount=(ncount>ncount1)?ncount:ncount1;
        return;
    }
    else
    {
        for(int i=1;i<=4;i++)//1向下，2向右，3向左
        {
            if(i==1&&x+1<=n&&f[x+1][y]!=1)
            {
                f[x+1][y]=1;
                ncount1+=t[x+1][y];
                walk(n,m,t,x+1,y);
                f[x+1][y]=0;
                ncount1-=t[x+1][y];
            }
            if(i==2&&y+1<=m&&f[x][y+1]!=1)
            {
                f[x][y+1]=1;
                ncount1+=t[x][y+1];
                walk(n,m,t,x,y+1);
                f[x][y+1]=0;
                ncount1-=t[x][y+1];
            }
            if(i==3&&y-1>=1&&f[x][y-1]!=1)
            {
                f[x][y-1]=1;
                ncount1+=t[x][y-1];
                walk(n,m,t,x,y-1);
                f[x][y-1]=0;
                ncount1-=t[x][y-1];
            }
        }
    }
}
int main() 
{
    int n,m;
    scanf("%d%d",&n,&m);
    int maze[10][10]={};
    for(int i=1;i<=n;i++)
        for(int j=1;j<=m;j++)
        scanf("%d",&maze[i][j]);
    f[1][1]=1;
    ncount1=maze[1][1];
    walk(n,m,maze,1,1);
    printf("%d",ncount);
    return 0;
}