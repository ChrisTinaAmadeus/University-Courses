#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    char week[7][10]={{"Monday"},{"Tuesday"},{"Wednesday"},{"Thursday"},{"Friday"},{"Saturday"},{"Sunday"}};
    int d1,d2;
    scanf("%d%d",&d1,&d2);
    for(int i=0;i<=9;i++)
    { 
        if(week[d1%7][i]!=0)
        printf("%c",week[d1%7][i]);
    }
    printf("\n");
    if(d2==1)
    d2=8;
    for(int j=0;j<=9;j++)
    { 
        if(week[d2-2][j]!=0)
        printf("%c",week[d2-2][j]);
    }
    return 0;
}