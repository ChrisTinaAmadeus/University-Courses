#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int power(int n)
{
    int sum=1;
    while(n>0)
    {
        sum*=10;
        n--;
    }
    return sum;
}
int main() 
{
    char num[10]={};
    int K;
    scanf("%s%*c%d",num,&K);
    int temp;
    int final=0,ncount=0;
    for(int i=strlen(num)-1;i>=0;i--)
    {
        if(num[i]!='X')final+=power(strlen(num)-1-i)*(num[i]-'0');
        else temp=i;
    }
    for(int i=0;i<10;i++)
    {
        if(!temp&&!i)continue;
        else
        {
            if((final+i*power(strlen(num)-1-temp))%K==0)
            ncount++;
        }
    }
    printf("%d",ncount);
    return 0;
}