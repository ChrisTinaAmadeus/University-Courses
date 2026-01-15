#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;

int main() 
{
    int N,M;
    scanf("%d%d",&N,&M);
    int flag[1500]={};
    int totalcount=0;
    int ncount=1;
    for(int j=1;;j++)
    {
        int i;
        if(j%N==0)i=N;
        else i=j%N;
        if(!(flag[i])&&ncount!=M)
        {
            ncount++;
            continue;
        }
        if(flag[i]) continue;
        if(ncount==M) 
        {
            flag[i]=1;
            ncount=1;
            totalcount++;
        }
        if(totalcount==N-1)break;
    }
    for(int i=1;i<=N;i++)
        if(flag[i]==0)
        {
            printf("%d",i);
            break;
        }
    return 0;
}