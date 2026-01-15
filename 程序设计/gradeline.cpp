#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n,m;
    scanf("%d%d",&n,&m);
    typedef struct
    {
        int sno;
        int grade;
        int order;
    }student;
    student students[5000]={};
    student temp;
    for(int i=0;i<n;i++)
        scanf("%d%d",&students[i].sno,&students[i].grade);
    int max;
    for(int i=0;i<n;i++)
        {
            max=i;
            for(int j=i+1;j<n;j++)
            {
                if(students[max].grade<students[j].grade)
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
    for(int i=1;i<n;i++)
    {
        if(students[i].grade==students[i-1].grade)
        students[i].order=students[i-1].order;
        else students[i].order=i+1;
    }
    int ncount=1,min;
    for(int i=1;i<=n;)//重点是小于等于，这样可以保证最后的数有排序，否则将直接跳出循环
    {
        if(students[i].order==students[i-1].order)
        {
            ncount++;
            i++;
            continue;
        }
        if(ncount!=1)
        {
            for(int j=i-ncount;j<i;j++)
            {
                min=j;
                for(int k=j+1;k<i;k++)
                {
                    if(students[min].sno>students[k].sno)
                    min=k;
                }
                if(min!=j)
                {
                    temp=students[j];
                    students[j]=students[min];
                    students[min]=temp;
                }
            }
            ncount=1;
        }
        else i++;
    }
    float M=m*1.5;
    int expect=int(M*1.0);
    int final=expect;
    for(int i=expect;i<n;i++)
    {
        if(students[i].grade==students[i-1].grade)
        final++;
        else break;
    }
    printf("%d %d\n",students[final-1].grade,final);
    for(int i=0;i<final;i++)
    printf("%d %d\n",students[i].sno,students[i].grade);
    return 0;
}