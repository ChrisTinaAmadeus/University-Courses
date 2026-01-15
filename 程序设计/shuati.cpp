#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,num[100]={};
    scanf("%d",&n);
    for(int i=0;i<n;i++)
        scanf("%d",&num[i]);
    int m,k;
    scanf("%d%d",&m,&k);
    typedef struct 
    {
        int sno;
        int p;
        int all[100]={};
        int allnum=0;
        int order;
    }student;
    student students[100]={};
    for(int i=0;i<m;i++)
    {
        scanf("%d%d",&students[i].sno,&students[i].p);
        for(int j=0;j<students[i].p;j++)
            scanf("%d",&students[i].all[j]);
    }
    for(int i=0;i<m;i++)
        for(int j=0;j<n;j++)
            for(int k=0;k<students[i].p;k++)
            {
                if(num[j]==students[i].all[k])
                students[i].allnum++;
            }
    int max;
    student temp;
    for(int i=0;i<m;i++)
    {
        max=i;
        for(int j=i+1;j<m;j++)
        {
            if(students[max].allnum<students[j].allnum)
            max=j;
        }
        if(max!=i)
        {
            temp=students[i];
            students[i]=students[max];
            students[max]=temp;
        }
    }
    students[0].order=1;
    for(int i=1;i<=m-1;i++)
    {
        if(students[i].allnum!=students[i-1].allnum)
        students[i].order=i+1;
        else students[i].order=students[i-1].order;
    }
    int mcount=1;
    for(int i=1;i<=m;)
    {
        if(students[i].order==students[i-1].order)
        {
            mcount++;
            i++;
            continue;
        }
        if(mcount!=1)
        {
            for(int l=i-mcount;l<i;l++)
            {
                int min=l;
                for(int k=l;k<i;k++)
                {
                    if(students[min].sno>students[k].sno)
                    min=k;
                }
                if(min!=l)
                {
                    temp=students[l];
                    students[l]=students[min];
                    students[min]=temp;
                }
            }
            mcount=1;
        }
        else i++;
    }
    printf("%d ",students[0].sno);
    int f=1;
    for(int i=1;i<m;i++)
    {
        if(students[i].order!=students[i-1].order)
        {
            f++;
        }
        if(f<=k)
        printf("%d ",students[i].sno);
        else break;
    }
    return 0;
}