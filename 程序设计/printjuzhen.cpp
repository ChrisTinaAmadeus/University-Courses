#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,m;
    scanf("%d%d",&n,&m);
    for(int i=1;i<=n;i++)
    {
        for(int j=0;j<m;j++)
        {
            printf("%d ",i+i*j);
        }
        printf("\n");
    }
}