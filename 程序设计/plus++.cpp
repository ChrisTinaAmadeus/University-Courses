#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int result,r;
long long power(int n)
{
    long long sum=1;
    while(n>0)
    {
        sum*=10;
        n--;
    }
    return sum;
}
void plus1(int k,int j,int n,char s[])
{
    if(k==n+1)
    {
        result=max(result,r);
        return;
    }
    else if(k==n)
    {
        int wei=strlen(s)-1-j,r1=0;
        while(wei>=0)
        {
            r+=power(wei)*(s[j]-'0');
            r1+=power(wei--)*(s[j++]-'0');
        }
        plus1(k+1,strlen(s),n,s);
        r-=r1;
    }
    else if(k!=n&&k!=n+1)
    {
        int wei=0,r1,j1=j;
        for(int i=j;i<strlen(s)-(n-k);i++)
        {
            wei=i-j,r1=0;
            while(wei>=0)
            {
                r1+=power(wei)*(s[j]-'0');
                r+=power(wei--)*(s[j++]-'0');
            }
            plus1(k+1,i+1,n,s);
            r-=r1;
            j=j1;
        }
    }
}
int main() 
{
    int m;
    char S[20]={};
    scanf("%d%*c%s",&m,S);
    plus1(1,0,m+1,S);
    printf("%d",result);
    return 0;
}