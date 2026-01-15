#include<stdio.h>
#include<iostream>
#include<string.h>
using namespace std;
int power(int a)
{
    int sum=1;
    for(a;a>0;a--)sum*=10;
    return sum;
}
int wei(int a)
{
    int zong=0,b=a;
    while(b>0)
    {
        zong++;
        b/=10;
    }
    return zong;
}
int ishuiwen(int a)
{
    int judge=1,judge2=1;
    if(wei(a)%2==0)judge2=0;
    int c=a;
    if(judge2)
    for(int i=0;;i++)
    {
        
        if(c<10)break;
        if(c%power(1)!=(c/power(wei(c)-1)))
        {
            judge=0;
            break;
        }

        c%=power(wei(c)-1);
        c/=10;
        if(wei(c)%2==0)
        {
            judge=0;
            break;
        }
    }
    else
    {
        
    for(int i=0;;i++)
    {
        if(c<10)break;
        if(c%power(1)!=(c/power(wei(c)-1)))
        {
            judge=0;
            break;
        }

        c%=power(wei(c)-1);
        c/=10;
        if(wei(c)%2!=0)
        {
            judge=0;
            break;
        }
    }
    }
    return judge;
}
int shuweihe(int a)
{
    int sum=0;
    for(int i=wei(a);i>0;i--)
    {
        sum+=a%10;
        a/=10;
    }
    return sum;
}
int main()
{
    int m,n;
    scanf("%d%d",&m,&n);
    int ma[2]={};
    int judge=0;
    for(int i=m;i<=n;i++)
    {

        if(ishuiwen(i))
        {
        int pan=1;
        for(int j=i-1;j>=2;j--)
        if(i%j==0)
        {
            pan=0;
            break;
        }
        if(pan)
        {
            judge=1;
            int initial=ma[1];
            ma[1]=max(ma[1],shuweihe(i));
            if(ma[1]!=initial)ma[0]=i;
        }
        }
    }
    if(judge)
    printf("%d %d",ma[0],ma[1]);
    else printf("0 0");
    return 0;
}