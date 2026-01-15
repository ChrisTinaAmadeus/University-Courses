#include <stdio.h>
#include <math.h>
#include<iostream>
using namespace std;
struct student
{
    int sno;
    int time;
    int should;
};
int main() 
{
    int n;
    student all[1000]={};
    scanf("%d",&n);
    for(int i=0;i<n;i++)
    {
        scanf("%d%d",&all[i].sno,&all[i].time);
        all[i].should=all[i].time+180;
    }
    student temp;
    for(int i=0;i<n;i++)
        for(int j=0;j<n-1;j++)
        {
            if(all[j].time>all[j+1].time)
            {
                temp=all[j];
                all[j]=all[j+1];
                all[j+1]=temp;
            }
            if(all[j].time==all[j+1].time)
            {
                if(all[j].sno>all[j+1].sno)
                {
                    temp=all[j];
                    all[j]=all[j+1];
                    all[j+1]=temp;
                }
            }
        }
    int num=0,k=0,I;
    student final[1000]={};
    for(int i=0;i<n&&(!k);i++)
    {
        num=0;
        for(int j=0;j<n;j++)
        {
            if(all[j].should<=all[i].should)num++;
            if(num>=n*0.3) 
            {
                k=1;
                I=i;
                break;
            }
        }
    }
    printf("%d\n",all[I].should);
    for(int i=0;i<num;i++)
    printf("%d %d\n",all[i].sno,all[i].time);
    for(int i=num;i<n;i++)
    {
        if(all[i].should==all[num-1].should)
        printf("%d %d\n",all[i].sno,all[i].time); 
    }
    return 0;
}