#include<stdio.h>
#include<cmath>
#include<iostream>
using namespace std;
#include<string.h>
int main()
{
    char S1[200]={},S2[200]={};
    fgets(S1,sizeof(S1),stdin);
    fgets(S2,sizeof(S2),stdin);
    if(strcmp(S1,S2)==0)
    printf("0");
    else
    {
        int i,j,k;
        for(i=0;i<=199;i++)
        {
            j=i;
            k=S1[i]-S2[j];
            if(S1[i]!=S2[j])
            {
                printf("%d",k);
                break;
            }
        }
    }
    return 0;
}