#include<stdio.h>
#include<string.h>
#include<math.h>
#include<iostream>
using namespace std;
typedef struct 
{
    int num[40];
    char past[40];
    char zeroout[40];
    long long now;
}math;
long long power(int a,int b)//计算a的b次方
{
    long long sum=1;
    while(b>0)
    {
        sum*=a;
        b--;
    }
    return sum;
}
long long change(char s[],char a[])//将外星数转化为十进制数
{
    int zero=strlen(s),k=0;
    long long sum=0;
    for(int i=strlen(a)-1;i>=0;i--)
        for(int j=0;j<zero;j++)
        {
            if(a[i]==s[j])
            {
                sum+=(j*power(zero,k));
                k++;
            }
        }
    return sum;
}
int main()
{
    math shu[105]={};
    int n;
    char standard[25]={};
    scanf("%d %s",&n,standard);
    for(int i=0;i<n;i++)
    {
        int mcount=0;
        scanf("%s",shu[i].past);
        for(int j=0;j<strlen(shu[i].past);j++)
            {
                if(shu[i].past[j]==standard[0])mcount++;
                else break;
            }
        if(mcount!=0)//去除首位的0
        {
            for(int j=0;j<strlen(shu[i].past);j++)
            shu[i].zeroout[j]=shu[i].past[j+mcount];
        }
        else strcpy(shu[i].zeroout,shu[i].past);
        for(int j=0;j<strlen(shu[i].zeroout);j++)
            for(int k=0;k<strlen(standard);k++)
            {
                if(shu[i].zeroout[j]==standard[k])shu[i].num[j]=k;
            }
    }
    math temp;
    for(int i=0;i<n;i++)
        for(int j=0;j<n-1;j++)
        {
            if(strlen(shu[j].zeroout)<strlen(shu[j+1].zeroout))
            {
                temp=shu[j];
                shu[j]=shu[j+1];
                shu[j+1]=temp;
            }
        }
    int ncount=1;
    for(int i=1;i<=n;)
    {
        if(strlen(shu[i].zeroout)==strlen(shu[i-1].zeroout))
        {
            ncount++;
            i++;
            continue;
        }
        if(ncount!=1)//等长度的字符串再排序
        {
            for(int l=strlen(shu[i-1].zeroout)-1;l>=0;l--)
                for(int j=i-ncount;j<i;j++)
                    for(int k=i-ncount;k<i-1;k++)
                    if(shu[k].num[l]<shu[k+1].num[l])
                    {
                        temp=shu[k];
                        shu[k]=shu[k+1];
                        shu[k+1]=temp;
                    }
            ncount=1;
        }
        else i++;
    }
    for(int i=0;i<n;i++)
    printf("%s ",shu[i].past);
    printf("\n%lld",change(standard,shu[n-1].past));
    return 0;
}