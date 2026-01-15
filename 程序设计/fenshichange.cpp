#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int power(int a)
{
    int sum=1;
    for(a;a>0;a--)
    sum*=10;
    return sum;
}
int main() 
{
    int N,D;
    scanf("%d%d",&N,&D);
    if((D%3==0||D%7==0)&&N%D!=0)
    {
        printf("%d.",N/D);
        N=N%D;
        long long n=(N*10000000000)/D;//10个0
        int num[15]={};
        for(int i=9;n>0;i--)
        {
            num[i]=n%10;
            n/=10;
        }
        int judge=1,ncountf,j;
        for(int ncount=1;judge;ncount++)
        {
            for(int i=0;i<10&&judge;i++)
            {
                if(ncount==1)
                {
                    if(num[i]==num[i+ncount]&&num[i+ncount]==num[i+2*ncount])
                    {
                        judge=0;
                        j=i;
                        ncountf=ncount;
                    }
                }
                if(ncount<=5&&ncount>1)
                {
                    if(num[i]==num[i+ncount]&&num[i+ncount]==num[i+2*ncount])
                    if(num[i+1]==num[i+ncount+1])
                    {
                        judge=0;
                        j=i;
                        ncountf=ncount;
                    }
                }
                else
                {
                    if(num[i]==num[i+ncount])
                    {
                        judge=0;
                        j=i;
                        ncountf=ncount;
                    }
                }
            }
        }
        for(int i=0;i<j;i++)printf("%d",num[i]);
        printf("(");
        for(int i=j;i<j+ncountf;i++)printf("%d",num[i]);
        printf(")");
    }
    else
    {
        if(N%D!=0)
        {
            printf("%d.",N/D);
            N=N%D;
            int n=(N*1000000)/D;
            int num[10]={};
            for(int i=5;n>0;i--)
            {
                num[i]=n%10;
                n/=10;
            }
            for(int i=0;;i++)
            {
                if(num[i]==0&&num[i+1]==0&&num[i+2]==0&&num[i+3]==0&num[i+4]==0)
                break;
                else
                printf("%d",num[i]);
            }
        }
        else 
        {
            double result=N/D;
            printf("%.1lf",result);
        }
    }
    return 0;
}