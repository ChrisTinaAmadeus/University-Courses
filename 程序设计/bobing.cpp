#include<stdio.h>
#include<iostream>
using namespace std;
int fen(int d[][6],int m)
{
    int final;
    int count[7]={};
    for(int i=1;i<7;i++)
        for(int j=0;j<6;j++)
        {
            if(d[m][j]==i)
            count[i]++;
        }
    if(count[4]==4)
    {
        if(count[1]==2) final=2048;
        else final=32;
    }
    else if(count[4]==3) final=8;
    else if(count[4]==2&&count[2]!=4)final=2;
    else if(count[4]==1&&count[2]!=5)
    {
        if(count[1]==1&&count[2]==1&&count[3]==1&&count[5]==1&&count[6]==1)
        final=16;
        else final=1;
    }
    else if(count[4]==5) final=128;
    else if(count[4]==6) final=1024;
    else if(count[1]==6) final=512;
    else if(count[2]==6) final=256;
    else if(count[2]==4) final=4;
    else if(count[2]==5)final=64;
    else final=0;
    return final;
}
int main()
{
    int n;
    scanf("%d",&n);
    int dian[256][6]={};
    int allfen=0,meici;
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<6;j++)
            scanf("%d",&dian[i][j]);
        meici=fen(dian,i);
        if(meici==0)
        {
            printf("%x",allfen);
            return 0;
        }
        allfen+=meici;
    }
    printf("%x",allfen);
    return 0;
}