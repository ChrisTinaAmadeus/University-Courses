#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
int result[3000];
int chengfa(char b[],char s[])
{
    int wei=strlen(s)-1;
    int blen=strlen(b);
    int maxl=blen+wei;//最长的情况
    
    for(int i=wei;i>=0;i--)//从最高位开始乘
    {
        int blen=strlen(b);
        int j;
        int jinweiplus=0;
        int jinweicheng=0;
        for(j=maxl-i;blen>=1;j--)
        {
            int ge=((b[blen-1]-'0')*(s[wei-i]-'0'))%10;
            //printf("%c %c\n",b[blen-1],s[wei-i]);
            int plus1=jinweiplus+jinweicheng+ge;
            result[j]+=plus1;
            jinweicheng=((b[blen-1]-'0')*(s[wei-i]-'0'))/10;
            jinweiplus=result[j]/10;
            result[j]%=10;
            blen--;
        }
        result[j]+=jinweiplus+jinweicheng;
        jinweiplus=result[j]/10;
        result[j]%=10;
        if(j>=1)result[j-1]+=jinweiplus;
    }
    return maxl;
}
int main()
{
    char m[300]={},n[300]={};
    scanf("%s %s",m,n);
    int length;
    if(strlen(m)>strlen(n))length=chengfa(m,n);
    else length=chengfa(n,m);
    if(m[0]=='0'||n[0]=='0') printf("0");
    else
    {
        if(result[0]!=0)printf("%d",result[0]);
        for(int i=1;i<=length;i++)
        printf("%d",result[i]);
    }
    return 0;
}