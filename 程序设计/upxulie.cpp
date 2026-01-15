#include<stdio.h>
#include<string.h>
#include<math.h>
#include<iostream>
using namespace std;
typedef struct 
{
    int str;
    int num[1050];
}xulie;
xulie s[110]={};
int judge(xulie ex)
{
   for(int i=0;i<ex.str-1;i++)
   {
        if(ex.num[i]>=ex.num[i+1])
        return i;
   }
   return ex.str;
}
int main()
{

    int m;
    scanf("%d",&m);
    for(int i=0;i<m;i++)
    {
        scanf("%d",&s[i].str);
        for(int k=0;k<s[i].str;k++)
        {
            scanf("%d",&s[i].num[k]);
        }
    }
    for(int i=0;i<m;i++)
    {
        if(judge(s[i])==s[i].str)printf("YES\n");
        else
        {
            for(int j=0;j<=judge(s[i]);j++)
            printf("%d ",s[i].num[j]);
            printf("\n");
        }
    }
    return 0;
}