#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
long long power(int s,int n)
{
    long long sum=1;
    while(n>0)
    {
        sum*=s;
        n--;
    }
    return sum;
}
long long change1(char s[],char a1[],int n1[])//化为十进制
{
    long long sum=0;
    int k=0;
    for(int i=strlen(a1)-1;i>=0;i--)
    {
        for(int j=0;j<strlen(s);j++)
        {
            if(a1[i]==s[j])n1[k++]=j;
        }
    }
    for(int i=0;i<strlen(a1);i++)
    {
        sum+=power(strlen(s),i)*n1[i];
    }
    return sum;
}
int change2(char s[],char a2[],long long n)//十进制化为目标进制，短除
{
    int i=0;
    while(n>=strlen(s))
    {
        a2[i++]=s[n%strlen(s)];//余数对应项从末尾开始
        n/=strlen(s);
    }
    a2[i]=s[n];
    return i;
}
int main() 
{
    int N;
    scanf("%d",&N);
    struct alien
    {
        int order;
        char initials[100];
        char finals[100];
        char initial[300];
        char final[300];
        int num[100];
        long long changenum;
        int ncount;
    };
    alien all[150]={};
    for(int i=0;i<N;i++)
    {
        scanf("%s%*c%s%*c%s",all[i].initial,all[i].initials,all[i].finals);
        all[i].changenum=change1(all[i].initials,all[i].initial,all[i].num);
        all[i].ncount=change2(all[i].finals,all[i].final,all[i].changenum);
        all[i].order=i+1;
    }
    for(int i=0;i<N;i++)
    {
        printf("Case #%d: ",all[i].order);
        for(int j=all[i].ncount;j>=0;j--)//倒着输出
        printf("%c",all[i].final[j]);
        printf("\n");
    }
    return 0;
}