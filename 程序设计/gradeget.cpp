#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int main() 
{
    int n,m,g;
    scanf("%d%d%d",&n,&m,&g);
    struct duan
    {
        int ncount;
        int order;
    };
    duan gradeduan[15]={};
    duan gradeduan2[15]={};
    int integral=100/m;
    int grade[6000]={};
    int ma=0;
    for(int i=0;i<n;i++)
    {
        scanf("%d",&grade[i]);
        for(int j=0;j<m;j++)
        {
            gradeduan[j].order=j;
            if(grade[i]>=j*integral&&grade[i]<(j+1)*integral)
            {
                gradeduan[j].ncount++;
                ma=max(ma,gradeduan[j].ncount);
                break;
            }
            if(grade[i]==100)
            {
                gradeduan[m-1].ncount++;
                ma=max(ma,gradeduan[j].ncount);
                break;
            }
        }
    }
    if(g==1)
    {
        duan temp;
        for(int i=0;i<m;i++)
            for(int j=0;j<m-1;j++)
            {
                if(gradeduan[j].ncount<gradeduan[j+1].ncount)
                {
                    temp=gradeduan[j];
                    gradeduan[j]=gradeduan[j+1];
                    gradeduan[j+1]=temp;
                }
                if(gradeduan[j].ncount==gradeduan[j+1].ncount&&gradeduan[j].order>gradeduan[j].order)
                {
                    temp=gradeduan[j];
                    gradeduan[j]=gradeduan[j+1];
                    gradeduan[j+1]=temp;
                }
            }
            for(int i=0;i<m;i++)
            {
                if(gradeduan[i].ncount==0)break;
                if(gradeduan[i].order==m-1)printf("[%d,100]: %d\n",gradeduan[i].order*integral,gradeduan[i].ncount);
                if(gradeduan[i].order*integral!=0&&gradeduan[i].order!=m-1)
                printf("[%d, %d): %d\n",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral,gradeduan[i].ncount);
                if(gradeduan[i].order*integral==0)
                printf("[ %d, %d): %d\n",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral,gradeduan[i].ncount);
            }
    }
    if(g==2)
    {
        if(ma<=50)
        {
            printf("[ 0, %d):",(gradeduan[0].order+1)*integral);
            for(int i=0;i<gradeduan[0].ncount;i++)
            printf("*");
            printf("\n");
            for(int i=1;i<m-1;i++)
            {
                printf("[%d, %d):",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral);
                for(int j=0;j<gradeduan[i].ncount;j++)
                printf("*");
                printf("\n");
            }
            printf("[%d,100]:",gradeduan[m-1].order*integral);
            for(int i=0;i<gradeduan[m-1].ncount;i++)
            printf("*");
        }
        else
        {
            for(int i=0;i<m;i++)
            {
                gradeduan[i].ncount*=50;
                gradeduan[i].ncount/=ma;
            }
            printf("[ 0, %d):",(gradeduan[0].order+1)*integral);
            for(int i=0;i<gradeduan[0].ncount;i++)
            printf("*");
            printf("\n");
            for(int i=1;i<m-1;i++)
            {
                printf("[%d, %d):",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral);
                for(int j=0;j<gradeduan[i].ncount;j++)
                printf("*");
                printf("\n");
            }
            printf("[%d,100]:",gradeduan[m-1].order*integral);
            for(int i=0;i<gradeduan[m-1].ncount;i++)
            printf("*");
        }
    }
    if(g==0)
    {
        for(int i=0;i<m;i++)
        gradeduan2[i]=gradeduan[i];
        duan temp;
        for(int i=0;i<m;i++)
            for(int j=0;j<m-1;j++)
            {
                if(gradeduan[j].ncount<gradeduan[j+1].ncount)
                {
                    temp=gradeduan[j];
                    gradeduan[j]=gradeduan[j+1];
                    gradeduan[j+1]=temp;
                }
                if(gradeduan[j].ncount==gradeduan[j+1].ncount&&gradeduan[j].order>gradeduan[j].order)
                {
                    temp=gradeduan[j];
                    gradeduan[j]=gradeduan[j+1];
                    gradeduan[j+1]=temp;
                }
            }
            for(int i=0;i<m;i++)
            {
                if(gradeduan[i].ncount==0)break;
                if(gradeduan[i].order==m-1)printf("[%d,100]: %d\n",gradeduan[i].order*integral,gradeduan[i].ncount);
                if(gradeduan[i].order*integral!=0&&gradeduan[i].order!=m-1)
                printf("[%d, %d): %d\n",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral,gradeduan[i].ncount);
                if(gradeduan[i].order*integral==0)
                printf("[ %d, %d): %d\n",gradeduan[i].order*integral,(gradeduan[i].order+1)*integral,gradeduan[i].ncount);
            }
        
        printf("\n");
        if(ma<=50)
        {
            printf("[ 0, %d):",(gradeduan2[0].order+1)*integral);
            for(int i=0;i<gradeduan2[0].ncount;i++)
            printf("*");
            printf("\n");
            for(int i=1;i<m-1;i++)
            {
                printf("[%d, %d):",gradeduan2[i].order*integral,(gradeduan2[i].order+1)*integral);
                for(int j=0;j<gradeduan2[i].ncount;j++)
                printf("*");
                printf("\n");
            }
            printf("[%d,100]:",gradeduan2[m-1].order*integral);
            for(int i=0;i<gradeduan2[m-1].ncount;i++)
            printf("*");
        }
        else
        {
            for(int i=0;i<m;i++)
            {
                gradeduan2[i].ncount*=50;
                gradeduan2[i].ncount/=ma;
            }
            printf("[ 0, %d):",(gradeduan2[0].order+1)*integral);
            for(int i=0;i<gradeduan2[0].ncount;i++)
            printf("*");
            printf("\n");
            for(int i=1;i<m-1;i++)
            {
                printf("[%d, %d):",gradeduan2[i].order*integral,(gradeduan2[i].order+1)*integral);
                for(int j=0;j<gradeduan2[i].ncount;j++)
                printf("*");
                printf("\n");
            }
            printf("[%d,100]:",gradeduan2[m-1].order*integral);
            for(int i=0;i<gradeduan2[m-1].ncount;i++)
            printf("*");
        }
    }
    return 0;
}