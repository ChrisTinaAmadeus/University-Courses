#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;

int main() 
{
    int N;
    scanf("%d",&N);
    if(N<10)
    {
        int j=2*N-1;
        for(int i=1;i<=N;i++)
        {
            for(int k=i-1;k>0;k--)
            {
                printf(" ");
            }
            for(int t=j;t>0;t--)
            printf("%d",N);
            j-=2;
            printf("\n");
        }
    }
    else
    {
        int m=N+55;
        int j=2*N-1;
        for(int i=1;i<=N;i++)
        {
            for(int k=i-1;k>0;k--)
            {
                printf(" ");
            }
            for(int t=j;t>0;t--)
            printf("%c",m);
            j-=2;
            printf("\n");
        }
    }
    return 0;
}