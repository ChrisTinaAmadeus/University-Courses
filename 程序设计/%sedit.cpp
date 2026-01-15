#include<stdio.h>
#include<string.h>
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
using namespace std;
int main()
{
    char natural[250]={};
    gets(natural);
    char order[101];
    gets(order);
    char a;
    char b[100]={},c[100]={},d[100]={};
    sscanf(order,"%c %s %s",&a,b,c);
    int e=strlen(b),N=strlen(natural),f=0,ncount=0;
    if(a=='C')// 统计子串s1在s0中[无重叠]的出现次数
    {
        for(int i=0;i<=201-e;i++)
        {
            f=0;
            for(int j=i;j<=i+e-1;j++)
            {
                d[f]=natural[j];
                f++;
            }
            if(strcmp(d,b)==0)
            {
                ncount++;
                i=i+e-1;
            }
        }
        printf("%d",ncount);
    }
    else if(a=='D')// 删除s0中的子串s1，若有多个子串s1，则删除第一次出现的
    {
        for(int i=0;i<=201-e;i++)
        {
            f=0;
            for(int j=i;j<=i+e-1;j++)
            {
                d[f]=natural[j];
                f++;
            }
            if(strcmp(d,b)==0)
            {
                for(int k=i;k<=201-e;k++)
                natural[k]=natural[k+e];
                break;
            }
        }
        printf("%s",natural);
    }
    else if(a=='I')// 表示将子串s2插入到子串s1的前面。若原串中有多个s1，则插入在最后一个子串的前面
    {
        for(int i=N-e+1;i>=0;i--)
        {
            f=1;
            for(int j=0;j<=e-1;j++)
            {
                if(natural[i+j]!=b[j])
                f=0;
            }
            if(f)
            {
                char g[100]={};
                for(int p=0;p+i<=N;p++)
                g[p]=natural[p+i];
                for(int q=i;q<=N;q++)
                natural[q]=0;
                strcat(natural,c);
                strcat(natural,g);
                break;
            }
        }
        printf("%s",natural);
    }
    else if(a=='R')// 在原字符串中用s2替换s1，s1为被替换的子串，2为替换的子串，若在原串中有多个s1则应全部替换。
    // 但当替换进去的子串与原串拼接后新出现子串str1时，不用再替换
    {
        int C=strlen(c);
        for(int i=0;i<=N+1-e;)
        {
            f=1;
            for(int j=0;j<=e-1;j++)
            {
                if(natural[i+j]!=b[j])
                f=0;
            }
            if(f)
            {
                char g[100]={};
                for(int p=0;p+i+e<=N;p++)
                {
                    g[p]=natural[p+e+i];
                }
                for(int q=i;q<=N;q++)
                {
                    natural[q]=0;
                }
                strcat(natural,c);
                strcat(natural,g);
                i+=C;
            }
            else i++;
        }
        printf("%s",natural);
    }
    return 0;
}