#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;

int main() 
{
    int n,m;
    scanf("%d %d",&n,&m);
    int num[110][110]={};
    int ma[110]={};
    long long maa=-100000000000;
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<m;j++)
        {
            scanf("%d",&num[i][j]);
            if(maa<num[i][j])maa=num[i][j];
        }
        ma[i]=maa;
        maa=-1000000000000;
    }
    long long mi=1000000000000;
    for(int i=0;i<n;i++)
    if(mi>ma[i])mi=ma[i];
    printf("%d",mi);
    return 0;
}