#include<stdio.h>
#include<iostream>
using namespace std;

int choose(int a[],int b,int c)
{
    int k;
    k=a[b];
    while(b!=c)
    {
        while((b<c)&&(a[c]>k))
        c--;
        if(b<c)
        {
            a[b]=a[c];
            b+=1;
        }
        while((b<c)&&(a[b]<=k))
        b++;
        if(b<c)
        {
            a[c]=a[b];
            c-=1;
        }
    }
    a[b]=k;
    return b; 
}
void qs(int n[],int l,int h)
{
    int m;
    if(l>=h)return;
    m=choose(n,l,h);
    qs(n,m+1,h);
    qs(n,l,m-1);
}
int main()
{
    int num[20]={};
    for(int i=0;i<20;i++)
    {
        scanf("%d",&num[i]);
    }
    qs(num,0,19);
    for(int i=0;i<20;i++)
    printf("%d ",num[i]);
    return 0;
}