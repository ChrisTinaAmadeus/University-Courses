#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int ncount,person,f[11],a1[11];
void distribute(int k,int n,int a[][11],int l[][11])
{
    if(k==n+1)
    {
        for(int i=0;i<n;i++)
        a[ncount][i]=a1[i];
        ncount++;
        return;
    }
    else
    {
        int judge=0;
        for(int i=0;i<n;i++)
        {
            if(f[i]!=1&&l[person][i])
            {
                judge=1;
                f[i]=1;
                a1[person]=i;
                person++;
                distribute(k+1,n,a,l);
                f[i]=0;
                person--;
                a1[person]=0;
            }
        }
        if(!judge)
        {
            return;
        }
    }
}
int main() 
{
    int n;
    scanf("%d",&n);
    int like[11][11]={};
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            scanf("%1d",&like[i][j]);
    int all[5000][11]={};
    distribute(1,n,all,like);
    if(ncount!=0)
    {
    printf("%d\n",ncount);
    for(int i=0;i<ncount;i++)
    {
        for(int j=0;j<n;j++)
        {
            printf("B%d",all[i][j]+1);
        }
        printf("\n");
    }
    }
    else printf("NO");
    return 0;
}