#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int N,M;
    scanf("%d%d",&N,&M);
    int xuefen[10]={};
    for(int i=0;i<M;i++)
        scanf("%d",&xuefen[i]);
    typedef struct 
    {
        int sno;
        int grade[10]={};
        int allget=0;
        int order;
    }student;
    student students[50]={};
    for(int i=0;i<N;i++)
    {
        scanf("%d",&students[i].sno);
        for(int j=0;j<M;j++)
            scanf("%d",&students[i].grade[j]);
    }
    for(int i=0;i<N;i++)
    {
        for(int j=0;j<M;j++)
        {
            if(students[i].grade[j]>=60)
            students[i].allget+=xuefen[j];
        }
    }
    int max;
    student temp;
    for(int i=0;i<N;i++)
    {
        max=i;
        for(int j=i+1;j<N;j++)
        {
            if(students[max].allget<students[j].allget)
            {
                max=j;
            }
        }
            if(max!=i)
            {
                temp=students[i];
                students[i]=students[max];
                students[max]=temp;
            }
    }
    students[0].order=1;
    for(int i=1;i<N;i++)
    {
        if(students[i].allget==students[i-1].allget)
        students[i].order=students[i-1].order;
        else students[i].order=i+1;
    }
    int f=1,min;
    for(int i=1;i<=N;)
    {
        if(students[i].order==students[i-1].order)
        {
            f++;
            i++;
            continue;
        }
        if(f!=1)
        {
            for(int j=i-f;j<i;j++)
            {
                min=j;
                for(int k=j;k<i;k++)
                    if(students[min].sno>students[k].sno)
                    {
                        min=k;
                    }
                if(min!=j)
                {
                    temp=students[j];
                    students[j]=students[min];
                    students[min]=temp;
                }
            }
            f=1;
        }
        else i++;
    }
    for(int i=0;i<N;i++)
    printf("%d %d\n",students[i].sno,students[i].allget);
        
}