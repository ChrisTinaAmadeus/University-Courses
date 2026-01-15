#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
int main()
{
    typedef struct 
    {
        char s[21];
        int num[21];
    }agct;
    agct all[1000]={};
    int N;
    scanf("%d",&N);
    char standard[5];
    scanf("%s",standard);
    for(int i=0;i<N;i++)
    {
        scanf("%s",all[i].s);
    }
    int l=strlen(all[0].s);
    for(int i=0;i<N;i++)
        for(int j=0;j<l;j++)
        {
            if(all[i].s[j]==standard[0])all[i].num[j]=4;
            if(all[i].s[j]==standard[1])all[i].num[j]=3;
            if(all[i].s[j]==standard[2])all[i].num[j]=2;
            if(all[i].s[j]==standard[3])all[i].num[j]=1;
        }
    agct temp;
    for(int j=l-1;j>=0;j--)
        for(int i=0;i<N;i++)
            for(int k=0;k<N-1;k++)
            {
                if(all[k].num[j]<all[k+1].num[j])
                {
                    temp=all[k];
                    all[k]=all[k+1];
                    all[k+1]=temp;
                }
            }
    for(int i=0;i<N;i++)
    {
        printf("%s\n",all[i].s);
    }
    return 0;
}