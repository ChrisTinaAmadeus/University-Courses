#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int classroom[1010][1010];
int m,n;
bool inside(int i,int ma)
{
    if(i<=ma&&i>=0)
    return true;
    else return false;
}
bool judge1(int x,int y)
{
    if(classroom[x][y]!=0) return false;
    if(inside(x-1,m-1)&&classroom[x-1][y]!=0)return false;
    else if(inside(x+1,m-1)&&classroom[x+1][y]!=0)return false;
    else if(inside(y-1,n-1)&&classroom[x][y-1]!=0&&classroom[x][y-1]!=1)return false;
    else if(inside(y+1,n-1)&&classroom[x][y+1]!=0&&classroom[x][y+1]!=1)return false;
    else return true;
}
int main() 
{
    scanf("%d%d",&m,&n);
    for(int i=0;i<m;i++)
        for(int j=0;j<n;j++)
        {
            scanf("%d",&classroom[i][j]);
        }
    for(int i=0;i<m;i++)
    if(classroom[0][i]!=1)
    {
        classroom[0][i]=2;
        break;
    }
    int ncount=1;
    for(int i=0;i<m;i++)
        for(int j=0;j<n;j++)
        {
            if(judge1(i,j))
            {
                classroom[i][j]=2;
                ncount++;
            }
        }
        printf("%d",ncount);
    return 0;
}