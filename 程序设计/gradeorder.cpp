#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    typedef struct
    {
        int order;
        int num;
        int grade;
    }student;
    student students[1000];
    student temp;
    int N;
    scanf("%d\n",&N);
    for(int i=0;i<N;i++)
        scanf("%d%d",&students[i].num,&students[i].grade);
    for(int i=0;i<=N-1;i++)
    {
        int max=i;
        for(int j=i;j<=N-1;j++)
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
    for(int i=1;i<=N-1;i++)
    {
        if(students[i].grade!=students[i-1].grade)
        students[i].order=i+1;
        else students[i].order=students[i-1].order;
    }
    int p=1;
    for(int i=1;i<=N;)//一定是<=N否则无法对最后的名次进行排序
    {
        if(students[i].order==students[i-1].order)
        {
            p++;
            i++;//最后一位同学判定结束后i还会加1，从N-1变为N
            continue;
        }
        if(p!=1)
        {
            for(int l=i-p;l<i;l++)
            {
                int min=l;
                for(int k=l;k<i;k++)
                {
                    if(students[min].num>students[k].num)
                    min=k;
                }
                if(min!=l)
                {
                    temp=students[l];
                    students[l]=students[min];
                    students[min]=temp;
                }
            }
            p=1;
        }
        else i++;
    }
    for(int i=0;i<N;i++)
    {
        printf("%d %d %d\n",students[i].order,students[i].num,students[i].grade);
    }
}