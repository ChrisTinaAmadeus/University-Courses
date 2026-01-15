#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int main() 
{
    char S[1050]={},S1[1050]={},S2[1050]={};
    scanf("%s%s%s",S,S1,S2);
    int s=strlen(S),s1=strlen(S1),s2=strlen(S2);
    int judge=1,left,right,finaljudge=1;
    for(int i=0;i<s-s1+1;i++)
    {
        int k=0;
        judge=1;
        for(int j=i;j<i+s1;j++)
        {
            if(S[j]!=S1[k])
            {
                judge=0;
                break;
            }
            k++;
        }
        if(judge)
        {
            left=i+s1;
            break;
        }
    }
    if(!judge) finaljudge=0;
    for(int i=s-s2;i>=0;i--)
    {
        int k=0;
        judge=1;
        for(int j=i;j<i+s2;j++)
        {
            if(S[j]!=S2[k])
            {
                judge=0;
                break;
            }
            k++;
        }
        if(judge)
        {
            right=i-1;
            break;
        }
    }
    if(right<left)judge=0;
    if(!judge)finaljudge=0;
    if(finaljudge)
    printf("%d",right-left+1);
    else printf("-1");
    return 0;
}