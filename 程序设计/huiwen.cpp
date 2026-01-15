#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
int main()
{
    char a[1000]={};//加一个“={}”可以使剩下没有赋值的全为0
    int i,j,k,l;
    scanf("%s",&a);
    char b[1000]={};
    for(i=0;i<=999;i++)
        b[i]=a[999-i];
    char c[1000]={};
    for(j=0;j<=999;j++)
        if(b[j]!=0)
        {
            k=j;
            break;
        }
    for(l=0;l<=999;l++)
    {
        c[l]=b[k];
        k++;
        if(k>999)
        break;
    }
if(strcmp(a,c)==0)
printf("Yes");
else printf("No");
return 0;
}