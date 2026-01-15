#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int a[101]={1,1};
    int i,j;
    for(i=3;i<=100;i++)
    {
        for(j=i-1;j>1;j--)
        {
            if(i%j==0)
            {
                a[i]=1;
                break;
            }
        }
    }
    for(int k=1;k<=100;k++)
        if(a[k]==0)
        printf("%d ",k);
    return 0;
}