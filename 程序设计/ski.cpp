#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int ncount=1,macount=1;
void ski(int h[][60],int m,int n,int x,int y)
{
    int judge=0;
    for(int i=1;i<=4;i++)
    {
        if(i==1&&y-1>=0)//左边
        {
            if(h[x][y-1]<h[x][y])
            {
                judge=1;
                ncount++;
                ski(h,m,n,x,y-1);
                ncount--;
            }
        }
        if(i==2&&x+1<m)//下边
        {
            if(h[x+1][y]<h[x][y])
            {
                judge=1;
                ncount++;
                ski(h,m,n,x+1,y);
                ncount--;
            }
        }
        if(i==3&&y+1<n)//右边
        {
            if(h[x][y+1]<h[x][y])
            {
                judge=1;
                ncount++;
                ski(h,m,n,x,y+1);
                ncount--;
            }
        }
        if(i==4&&x-1>=0)//上边
        {
            if(h[x-1][y]<h[x][y])
            {
                judge=1;
                ncount++;
                ski(h,m,n,x-1,y);
                ncount--;
            }
        }
    }
    if(!judge)
    {
        macount=max(macount,ncount);
        return;
    }
}
int main() 
{
    int m,n;
    scanf("%d%d",&m,&n);
    int high[60][60]={};
    for(int i=0;i<m;i++)
        for(int j=0;j<n;j++)
        {
            scanf("%d",&high[i][j]);
        }
    for(int i=0;i<m;i++)
        for(int j=0;j<n;j++)
        ski(high,m,n,i,j);
    printf("%d",macount);
    return 0;
}