#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int partition(long long a[],long long d[],int l,int r)
{
    long long k=d[l],K=a[l];
    while(l!=r)
    {
        while((l<r)&&((d[r]<k)||(d[r]==k&&a[r]<K)))
        r--;
        if(l<r)
        {
            d[l]=d[r];
            a[l]=a[r];
            l+=1;
        }
        while((l<r)&&((d[l]>k)||(d[l]==k&&a[l]>=K)))
        l++;
        if(l<r)
        {
            d[r]=d[l];
            a[r]=a[l];
            r-=1;
        }
    }
    d[l]=k;
    a[l]=K;
    return l;
}
void qs(long long a[],long long d[],int l,int r)
{
    if(l>r)return;
    int m=partition(a,d,l,r);
    qs(a,d,l,m-1);
    qs(a,d,m+1,r);
}
int main() 
{
    int n;
    scanf("%d",&n);
    long long all[101000]={};
    long long date[101000]={};
    for(int i=0;i<n;i++)
    {
        scanf("%lld",&all[i]);
        date[i]=(all[i]/10000)%100000000;
    }
    qs(all,date,0,n-1);
    for(int i=0;i<n;i++)
    {
        printf("%lld\n",all[i]);
    }
    return 0;
}