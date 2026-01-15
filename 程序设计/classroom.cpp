#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int Na,Nb,Nc,Nd,n=0;
    scanf("%d%d%d%d",&Na,&Nb,&Nc,&Nd);
    int classroom[9]={0,120,40,85,50,100,140,70,100};
    for(int i=1;i<=8;i++)
        for(int j=1;j<=8;j++)
            for(int k=1;k<=8;k++)
                for(int l=1;l<=8;l++)
                {
                    if(Na<=classroom[i]&&Nb<=classroom[j]&&Nc<=classroom[k]&&Nd<=classroom[l])
                        if(i!=j&&i!=k&&i!=l&&j!=k&&j!=l&&k!=l)
                        {
                            n=1;
                            printf("%d %d %d %d\n",i,j,k,l);
                        }
                }
    if(!n) printf("-1");
    return 0;
}