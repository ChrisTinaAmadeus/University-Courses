#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int m,n;
    scanf("%d%d\n",&m,&n);
    int num[102][102]={};
    for(int i=1;i<=m;i++)
        for(int j=1;j<=n;j++)
        {
            scanf("%d",&num[i][j]);
        }
    int b=0,d=0,e[100][100]={};
    for(int k=1;k<=m;k++)
        for(int l=1;l<=n;l++)
            {
                e[k-1][l-1]=num[k][l]+num[k-1][l]+num[k][l-1]+num[k][l+1]+num[k+1][l];
            }
    for(int I=0;I<=m-1;I++)
        for(int J=0;J<n-1;J++)
        {
            if(b<e[I][J])
            b=e[I][J];
        }
    for(int K=0;K<=m-1;K++)
        for(int L=0;L<=n-1;L++)
        {
            if(e[K][L]==b)
            d++;
        }
    printf("%d %d\n",b,d);
    for(int k=1;k<=m;k++)
        for(int l=1;l<=n;l++)
            {
                if(num[k][l]+num[k-1][l]+num[k][l-1]+num[k][l+1]+num[k+1][l]==b)
                printf("%d %d\n",k-1,l-1);
            }
    return 0;
}