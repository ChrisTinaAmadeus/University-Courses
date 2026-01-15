#include<stdio.h>
#include<cmath>
#include<string.h>
#include<iostream>
using namespace std;
int main()
{
    int y[1000],t,sum,p=0;
    for(int i=0;i<1000;i++)
    {
        scanf("%d",&y[i]);
        if(y[i]==0)
        {
            t=i-1;
            break;
        }
    }
    for(int j=0;j<=t;j++)
    {
        for(int x=1;x<=100000;x++)
        {
            p=0;
            int m=x;
            sum=x;
            while(m!=0)
            {
                sum+=m%10;
                m=m/10;
            }
            if(sum==y[j])
            {
            printf("%d\n",x);
            p=1;
            break;
            }
        }
        if(!p)
        printf("0\n");
    }
    return 0;
}