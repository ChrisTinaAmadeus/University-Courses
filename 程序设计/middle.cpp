#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,find=-1;
    int m;
    int num[1000]={};
    scanf("%d%d",&n,&m);
    for(int i=0;i<=n-1;i++)
        scanf("%d",&num[i]);
    int temp;
    for(int k=0;k<=n-2;k++)
        for(int j=0;j<=n-2;j++)
            if(num[j]>num[j+1])
            {
                temp=num[j];
                num[j]=num[j+1];
                num[j+1]=temp;
            }
    int left=0,right=n-1,mid,ncount=0;
    while(left<=right)
    {
        ncount++;
        mid=(left+right)/2;
        if(num[mid]==m)
        {
            find=mid;
            break;
        }
        else if(num[mid]<m)
        left=mid+1;
        else right=mid-1;
    }
    printf("%d\n%d",find,ncount);
    return 0;
}