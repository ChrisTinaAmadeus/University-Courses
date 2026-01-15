#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,k;
    scanf("%d%d",&n,&k);
    typedef struct
    {
        int num;
        int all;
    }student;
    student students[180]={};
    for(int i=0;i<n;i++)
        scanf("%d",&students[i].num);
    for(int i=0;i<n;i++)
    {
        int m=students[i].num,sum=0;
        while(m!=0)
        {
            sum+=m%10;
            m=m/10;
        }
        students[i].all=sum;
    }
    int ncount=0,l=0;
    int allpeople[180]={};
    for(int i=0;i<n;i++)
    {
        if(students[i].all%k==0)
        {
            ncount++;
            allpeople[l]=students[i].num;
            l++;
        }
    }
    int temp;
    for(int i=0;i<ncount;i++)
    {
        int min=i;
        for(int p=i;p<ncount;p++)
        {
            if(allpeople[min]>allpeople[p])
            min=p;
        }
        if(min!=i)
        {
            temp=allpeople[i];
            allpeople[i]=allpeople[min];
            allpeople[min]=temp;
        }
    }
    printf("%d\n",ncount);
    for(int i=0;i<ncount;i++)
        printf("%d\n",allpeople[i]);
    return 0;
}