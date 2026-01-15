#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,a,b,c,A,B,C;
    scanf("%d\n%d%d%d\n%d%d%d",&n,&a,&b,&c,&A,&B,&C);
    for(int i=0;i<=A;i++)
        for(int j=0;j<=B;j++)
            for(int k=0;k<=C;k++)
            {
                if(a*i+b*j+c*k==n)
                printf("%d %d %d\n",i,j,k);
            }
    return 0;
}