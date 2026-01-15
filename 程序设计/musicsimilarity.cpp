#include <stdio.h>
#include <string.h>
#include<cmath>
#include<iostream>
using namespace std;
void times(int a[],int b[],int c)
{
    for(int i=0;i<c;i++)
    {
        if(a[i]>=0&&a[i]<=15)b[0]++;
        if(a[i]>=16&&a[i]<=31)b[1]++;
        if(a[i]>=32&&a[i]<=47)b[2]++;
        if(a[i]>=48&&a[i]<=63)b[3]++;
        if(a[i]>=64&&a[i]<=79)b[4]++;
        if(a[i]>=80&&a[i]<=95)b[5]++;
        if(a[i]>=96&&a[i]<=111)b[6]++;
        if(a[i]>=112&&a[i]<=127)b[7]++;
        if(a[i]>=128&&a[i]<=143)b[8]++;
        if(a[i]>=144&&a[i]<=159)b[9]++;
        if(a[i]>=160&&a[i]<=175)b[10]++;
        if(a[i]>=176&&a[i]<=191)b[11]++;
        if(a[i]>=192&&a[i]<=207)b[12]++;
        if(a[i]>=208&&a[i]<=223)b[13]++;
        if(a[i]>=224&&a[i]<=239)b[14]++;
        if(a[i]>=240&&a[i]<=255)b[15]++;
    }
}
double juli(int a[],int b[])
{
    int sum=0;
    double result;
    for(int i=0;i<16;i++)
    {
        sum+=(a[i]-b[i])*(a[i]-b[i]);
    }
    result=sqrt(sum);
    return result;
}
int main() 
{
    struct music
    {
        int sno;
        int length;
        int f[110];
        int t[20];
        double similarity;
    };
    music all[110]={};
    music standard;
    scanf("%d",&standard.length);
    for(int i=0;i<standard.length;i++)
    {
        scanf("%d",&standard.f[i]);
    }
    times(standard.f,standard.t,standard.length);
    int n,k;
    scanf("%d%d",&n,&k);
    for(int i=0;i<n;i++)
    {
        scanf("%d",&all[i].length);
        all[i].sno=i;
        for(int j=0;j<all[i].length;j++)
        {
            scanf("%d",&all[i].f[j]);
        }
        times(all[i].f,all[i].t,all[i].length);
        all[i].similarity=juli(standard.t,all[i].t);
    }
    music temp;
    for(int i=0;i<n;i++)
        for(int j=0;j<n-1;j++)
        {
            if(all[j].similarity>all[j+1].similarity)
            {
                temp=all[j];
                all[j]=all[j+1];
                all[j+1]=temp;
            }
            if(all[j].similarity==all[j+1].similarity)
            {
                if(all[j].sno>all[j+1].sno)
                {
                    temp=all[j];
                    all[j]=all[j+1];
                    all[j+1]=temp;
                }
            }
        }
    for(int i=0;i<k;i++)
    printf("%d ",all[i].sno);
    return 0;
}