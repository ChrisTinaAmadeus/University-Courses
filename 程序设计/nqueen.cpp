#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int possibility;
void conflict(int i,int j,int n,int chess[15][15])
{
        //斜线加1
            int k1=1,k2=1,k3=1,k4=1;
            while((i-k1>=0)&&(j-k1>=0))//左上
            {
                chess[i-k1][j-k1]++;
                k1++;
            }
            while((i-k2>=0)&&(j+k2<n))//右上
            {
                chess[i-k2][j+k2]++;
                k2++;
            }
            while((i+k3<n)&&(j-k3>=0))//左下
            {
                chess[i+k3][j-k3]++;
                k3++;
            }
            while((i+k4<n)&&(j+k4<n))//右下
            {
                chess[i+k4][j+k4]++;
                k4++;
            }
        //行列加1
        for(int k=0;k<n;k++)
        {
            chess[i][k]++;
            chess[k][j]++;
        }
        chess[i][j]-=1;
}
void conflictback(int i,int j,int n,int chess[15][15])
{
        //斜线减1
            int k1=1,k2=1,k3=1,k4=1;
            while((i-k1>=0)&&(j-k1>=0))//左上
            {
                chess[i-k1][j-k1]--;
                k1++;
            }
            while((i-k2>=0)&&(j+k2<n))//右上
            {
                chess[i-k2][j+k2]--;
                k2++;
            }
            while((i+k3<n)&&(j-k3>=0))//左下
            {
                chess[i+k3][j-k3]--;
                k3++;
            }
            while((i+k4<n)&&(j+k4<n))//右下
            {
                chess[i+k4][j+k4]--;
                k4++;
            }
        //行列减1
        for(int k=0;k<n;k++)
        {
            chess[i][k]--;
            chess[k][j]--;
        }
        chess[i][j]+=1;
}
void finaljudge(int k,int n,int chess[15][15],int I)
{
    if(k==n+1)
    {
        possibility++;
        return;
    }
    for(int j=0;j<n;j++)
    {
        if(chess[I][j]==0)
        {
            conflict(I,j,n,chess);
            finaljudge(k+1,n,chess,I+1);
            conflictback(I,j,n,chess);
        }
    }
}
int main() 
{
    int n;
    scanf("%d",&n);
    int chess[15][15]={};
    finaljudge(1,n,chess,0);
    printf("%d",possibility);
    return 0;
}