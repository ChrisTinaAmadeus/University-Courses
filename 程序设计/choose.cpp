#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;

int main() 
{
    int n;
    scanf("%d",&n);
    int num[110]={};
    int ncount=0;
    for(int i=0;i<n;i++)
    scanf("%d",&num[i]);
    for(int i=0;i<n;i++)
    {
        if(num[i]>=0)ncount+=num[i];
    }
    printf("%d",ncount);
    return 0;
}