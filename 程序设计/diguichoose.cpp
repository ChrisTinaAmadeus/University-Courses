#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int steps,temp;
void choose(int ary[],int n[],int i,int all)
{
    if(i==all)return;
    int min=i;
    for(int j=i+1;j<=all;j++)
    {
        if(ary[min]>ary[j]) min=j;
    }
    if(min!=i)
    {
        temp=ary[min];
        ary[min]=ary[i];
        ary[i]=temp;
        steps++;
        for(int j=1;j<=all;j++)
        n[j]=ary[j];
        choose(ary,n,i+1,all);
        printf("%d<->%d:",i,min);
        for(int j=1;j<=all;j++)
        {
            printf("%d ",n[j]);
        }
        temp=n[min];
        n[min]=n[i];
        n[i]=temp;
        printf("\n");
    }
    else choose(ary,n,i+1,all);
}
int main() 
{
    int n;
    scanf("%d",&n);
    int a[110]={},num[110]={};
    for(int i=1;i<=n;i++)
    {
        scanf("%d",&a[i]);
    }
    choose(a,num,1,n);
    printf("Total steps:%d\n",steps);
    printf("Right order:");
    for(int i=1;i<=n;i++)
    printf("%d ",a[i]);
    return 0;
}