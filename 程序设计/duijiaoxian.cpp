#include<stdio.h>
#include<string.h>
#include<math.h>
#include<iostream>
using namespace std;
int main()
{
    int N;
    scanf("%d",&N);
    int num[150][150]={};
    for(int i=0;i<N;i++)
        for(int j=0;j<N;j++)
        {
            scanf("%d",&num[i][j]);
        }
    int min=num[0][0];
    for(int i=0;i<N;i++)
    if(min>num[i][i])
    min=num[i][i];
    int k=N-1;
    for(int i=0;i<N;i++)
    {
    if(min>num[i][k])
    min=num[i][k];
    k--;
    }
    printf("%d",min);
    return 0;
}