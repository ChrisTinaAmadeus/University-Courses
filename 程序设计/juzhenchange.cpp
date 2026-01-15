#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int main() 
{
    int n,judge1=1,judge2=1,judge3=1,judge4=1;
    scanf("%d",&n);
    int initial[55][55]={},final[55][55]={};
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
        scanf("%d",&initial[i][j]);
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
        scanf("%d",&final[i][j]);
    int m1=n-1;
    for(int i=0;i<n&&judge1;i++)
    {
        for(int j=0;j<n;j++)
        {
            if(initial[i][j]!=final[j][m1])
            {
                judge1=0;
                break;
            }
        }
        m1--;
    }
    int m2=n-1;
    for(int i=0;i<n&&judge2;i++)
    {
        m2=n-1;
        for(int j=0;j<n;j++)
        {
            if(initial[i][j]!=final[m2][i])
            {
                judge2=0;
                break;
            }
            m2--;
        }
    }
    for(int i=0;i<n&&judge4;i++)
        for(int j=0;j<n&&judge4;j++)
        {
            if(initial[i][j]!=final[i][j])judge4=0;
        }
    for(int i=0;i<n&&judge3;i++)
        for(int j=0;j<n&&judge3;j++)
        {
            if(initial[i][j]!=final[n-1-i][n-1-j])
            judge3=0;
        }
    if(judge1)printf("1");
    if(judge2)printf("2");
    if(judge3)printf("3");
    if(judge4)printf("4");
    if(!judge1&&!judge2&&!judge3&&!judge4) printf("5");
    return 0;
}