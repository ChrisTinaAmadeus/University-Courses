#include<stdio.h>
#include<string.h>
#include<iostream>
using namespace std;
int strl(int m[])
{
    int sum=0;
    for(int i=0;i<20;i++)
    {
        if(m[i]!=-1)
        sum++;
        else break;
    }
    return sum;
}
int main()
{
    int a[20]={},b[20]={};
    for(int i=0;;i++)
    {
        scanf("%d",&a[i]);
        if(a[i]==-1)
        break;
    }
    for(int i=0;;i++)
    {
        scanf("%d",&b[i]);
        if(b[i]==-1)
        break;
    }
    printf("%d\n%d\n",strl(a),strl(b));
    int k=0;
    for(int i=0;i<min(strl(a),strl(b));i++)
    {
        if(a[i]==b[i]) 
        k++;
        else
        {
            k=-1;
            printf("%d",a[i]-b[i]);
            break;
        }
    }
    if(k==min(strl(a),strl(b))&&strl(a)==strl(b)) printf("0");
    else if(k!=-1)printf("Not Comparable");
    return 0;
}