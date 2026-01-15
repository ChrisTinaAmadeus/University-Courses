#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
/*没有考虑减法多位借1和加法多位进1的情况，
并且减法的一个分支没有考虑首位为负号的情况直接进行了字符串长度比较，
建议参考另一个减法的做法来自行脑补，苯人已经不想再碰这个sb题了*/
int one(char a1,char a2,int a3)
{
    int judge=0;
    if(a1-'0'+a2-'0'+a3-10>=0)judge=1;
    return judge;
}
int zheng(char a1,char a2,int a3,int n,int s[])
{
    int sum;
    sum=a1-a2-a3;
    if(sum<0)
    {
        sum+=10;
        s[n]=1;
    }
    return sum;
}
void plus1(char n1[],char n2[],int r[])
{
    int l1=strlen(n1)-1,l2=strlen(n2)-1;
    int lr=max(l1,l2)+1;
    int shi[3000]={};
    while((l1>=1)&&(l2>=1))
    {
        if(one(n1[l1],n2[l2],shi[lr-1]))
        {
            r[lr]=n1[l1--]-'0'+n2[l2--]-'0'+shi[lr-1]-10;
            shi[lr-2]=1;
            lr--;
        }
        else r[lr--]=n1[l1--]-'0'+n2[l2--]-'0'+shi[lr-1];
    }
    if(l1>=1)
    {
        if(n2[0]=='-')
        {
        for(int i=lr;i>=2;i--)
        r[i]=n1[l1--]-'0'+shi[i-1];
        }
        else
        {
            if(one(n1[l1],n2[0],shi[lr-1]))
            {
                r[lr]=n1[l1--]-'0'+n2[0]-'0'+shi[lr-1]-10;
                shi[lr-2]=1;
                lr-=1;
            }
            else r[lr--]=n1[l1--]-'0'+n2[0]-'0'+shi[lr-1];
            for(int i=lr;i>=2;i--)
            r[i]=n1[l1--]-'0'+shi[i-1];
        }
        if(n1[0]!='-')
        {
            r[1]=n1[0]-'0'+shi[0];
        }
    }
    else if(l2>=1)
    {
        if(n1[0]=='-')
        {
        for(int i=lr;i>=2;i--)
        r[i]=n2[l2--]-'0'+shi[i-1];
        }
        else
        {
            if(one(n1[0],n2[l2],shi[lr-1]))
            {
                r[lr]=n1[0]-'0'+n2[l2--]-'0'+shi[lr-1]-10;
                shi[lr-2]=1;
                lr-=1;
            }
            else r[lr--]=n1[0]-'0'+n2[l2--]-'0'+shi[lr-1];
            for(int i=lr;i>=2;i--)
            r[i]=n2[l2--]-'0'+shi[i-1];
        }
        if(n2[0]!='-')
        {
            r[1]=n2[0]-'0'+shi[0];
        }
    }
    else if(l1==l2)
    {
        if(n1[0]!='-'&&n2[0]!='-')
        {
            if(one(n1[0],n2[0],shi[0]))
            {
                r[0]=1;
                r[1]=n1[0]-'0'+n2[0]-'0'-10+shi[0];
            }
            else r[1]=n1[0]-'0'+n2[0]-'0'+shi[0];
        }
        else if(n1[0]!='-'&&n2[0]=='-')
        r[1]=n1[0]-'0'+shi[0];
        else if(n2[0]!='-'&&n1[0]=='-')
        r[1]=n2[0]-'0'+shi[0];
    }
}
void minus1(char n1[],char n2[],int r[])//输入时保证n1长度大于等于n2
{
    int l1=strlen(n1)-1,l2=strlen(n2)-1;
    int lr=max(l1,l2)+1;
    int shi[3000]={};
    while((l1>=1)&&(l2>=1))
    {
        r[lr--]=zheng(n1[l1--],n2[l2--],shi[lr-1],lr-2,shi);
    }
    if(l1>=1)
    {
        if(n2[0]=='-')
        {
        for(int i=lr;i>=2;i--)
        r[i]=n1[l1--]-'0'-shi[i-1];
        }
        else
        {
            r[lr--]=zheng(n1[l1--],n2[l2--],shi[lr-1],lr-2,shi);
            for(int i=lr;i>=2;i--)
            r[i]=n1[l1--]-'0'-shi[i-1];
        }
        if(n1[0]!='-')
        {
            r[1]=n1[0]-'0'-shi[0];
        }
    }
    else if(l1==l2)
    {
        if(n1[0]!='-'&&n2[0]!='-')
        {
            r[1]=zheng(n1[0],n2[0],shi[0],2500,shi);
        }
        else if(n1[0]!='-'&&n2[0]=='-')
        r[1]=n1[0]-'0'-shi[0];
        else if(n2[0]!='-'&&n1[0]=='-')
        r[1]=n2[0]-'0'-shi[0];
    }
}
int main() 
{
    char fuhao;
    scanf("%c",&fuhao);
    char num1[3000]={},num2[3000]={};
    int result[3000]={};
    scanf("%s",num1);
    scanf("%s",num2);
    int judge1=1,judge2=1,judge3=1;
    if(num1[0]=='-')judge1=0;
    if(num2[0]=='-')judge2=0;
    if(fuhao=='-')judge3=0;
    if(judge1==judge2&&judge3) //加法（同正或者同负情况）
    {
        plus1(num1,num2,result);
        if(!judge1)
        {
            printf("-");
        }
        if(result[0]!=0)printf("%d",result[0]);
        if((result[1]!=0&&result[0]==0)||(result[0]!=0))printf("%d",result[1]);
        for(int i=2;i<=max(strlen(num1),strlen(num2));i++)
        printf("%d",result[i]);
    }
    if(!judge3&&(judge1!=judge2))//减法（一正一负情况）
    {
        plus1(num1,num2,result);
        if(judge1==0)
        {
            printf("-");
            if(result[0]!=0)printf("%d",result[0]);
            if((result[1]!=0&&result[0]==0)||(result[0]!=0))printf("%d",result[1]);
            for(int i=2;i<=max(strlen(num1),strlen(num2));i++)
            printf("%d",result[i]);
        }
        if(judge2==0)
        {
            if(result[0]!=0)printf("%d",result[0]);
            if((result[1]!=0&&result[0]==0)||(result[0]!=0))printf("%d",result[1]);
            for(int i=2;i<=max(strlen(num1),strlen(num2));i++)
            printf("%d",result[i]);
        }
    }
    if(judge3&&(judge1!=judge2))//加法（一正一负情况）
    {
        if(num1[0]=='-')
        {
            if(strlen(num1)-1>=strlen(num2))
            {
                minus1(num1,num2,result);
                if(!judge1)printf("-");
                if(result[1]!=0)printf("%d",result[1]);
                if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
                for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
                printf("%d",result[i]);
            }
            else 
            {
                minus1(num2,num1,result);
                if(judge1)printf("-");
                if(result[1]!=0)printf("%d",result[1]);
                if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
                for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
                printf("%d",result[i]);
            }
        }
        else
        {
            if(strlen(num1)>=strlen(num2)-1)
            {
                minus1(num1,num2,result);
                if(!judge1)printf("-");
                if(result[1]!=0)printf("%d",result[1]);
                if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
                for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
                printf("%d",result[i]);
            }
            else 
            {
                minus1(num2,num1,result);
                if(judge1)printf("-");
                if(result[1]!=0)printf("%d",result[1]);
                if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
                for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
                printf("%d",result[i]);
            }
        }
    }
    if(!judge3&&(judge1==judge2))//减法（同正同负情况）
    {
        if(strlen(num1)>=strlen(num2))
        {
            minus1(num1,num2,result);
            if(!judge1)printf("-");
            if(result[1]!=0)printf("%d",result[1]);
            if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
            for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
            printf("%d",result[i]);
        }
        else 
        {
            minus1(num2,num1,result);
            if(judge1)printf("-");
            if(result[1]!=0)printf("%d",result[1]);
            if((result[2]!=0&&result[1]==0)||(result[1]!=0))printf("%d",result[2]);
            for(int i=3;i<=max(strlen(num1),strlen(num2));i++)
            printf("%d",result[i]);
        }
    }
    return 0;
}