#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int flag[600],ncount;
char b[600];
int I;
void pailie(char a[],int n,int k)
{
    if(k==n)
    {
        printf("%s\n",b);
        ncount++;
        return;
    }
    else
    {
        int pick=0;
        for(int i=0;i<n;i++)
        {
            if(flag[i]!=1&&((a[I]!=a[i]&&pick)||!pick))
            {
                flag[i]=1;
                b[k]=a[i];
                pick=1;
                pailie(a,n,k+1);
                flag[i]=0;
                I=i;
            }
        }
    }
}
int main() 
{
    int n;
    scanf("%d",&n);
    char all[600]={};
    scanf("%s",all);
    //按字典序先排序
    char temp;
    for(int i=0;i<n;i++)
        for(int j=0;j<n-1;j++)
        {
            if(all[j]>all[j+1])
            {
                temp=all[j];
                all[j]=all[j+1];
                all[j+1]=temp;
            }
        }
    pailie(all,n,0);
    printf("%d",ncount);
    return 0;
}