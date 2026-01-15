#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int main() 
{
    int n;
    scanf("%d",&n);
    struct num
    {
        int a;
        int ncount;
    };
    num all[1000]={};
    int mcount=0;
    for(int i=2;i<=n;i++)
    {
        int judge=1;
        for(int j=i-1;j>=2&&judge;j--)
        {
            if(i%j==0) judge=0;
        }
        if(judge&&n%i==0)
        {
            all[mcount].a=i;
            while(n%i==0)
            {
                all[mcount].ncount++;
                n/=i;
            }
            mcount++;
        }
    }
    for(int i=0;i<mcount;i++)
    {
        printf("%d:%d\n",all[i].a,all[i].ncount);
    }
    return 0;
}