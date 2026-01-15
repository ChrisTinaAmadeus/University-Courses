#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int N,x,y,z=0,k=0,o=0,sum=0,number;
    scanf("%d",&N);//输入行数
    int num[10]={};
    int a[10]={},b[10]={},c[500]={};
    for(int i=0;i<=N-1;i++)
    scanf("%d %d %d",&num[i],&a[i],&b[i]);//分别将输入的值计入三个不同的数组
    for(x=1000;x<10000;x++)//枚举找满足第二个条件的数
    {
        k=0;
        for(int j=0;j<=N-1;j++)
        {
        if((x/1000==num[j]/1000)+((x%1000)/100==(num[j]%1000)/100)+(((x%1000)%100)/10==((num[j]%1000)%100)/10)+(x%10==num[j]%10)==b[j])
        k++;
        }
        if(k==N)
        {
            c[o]=x;//将满足第二个条件的数放入c数组
            o++;
        }
    }
    int d[10][4],e[500][4];//将a和c的每位数字放入d和e数组
    for(int I=0;I<=9;I++)
    {
        int K=num[I];
        for(int J=3;J>=0;J--)
            {
                d[I][J]=K%10;
                K=K/10;
            }
    }
    for(int A=0;A<=490;A++)
    {
        int B=c[A];
        for(int C=3;C>=0;C--)
            {
                e[A][C]=B%10;
                B=B/10;
            }
    }
    int f[10]={0,1,2,3,4,5,6,7,8,9};
    int g[10][10]={},h[500][10]={};//数数字有几个
    for(int I=0;I<=N-1;I++)
        for(int J=0;J<=3;J++)
            for(int K=0;K<=9;K++)
                {
                    if(d[I][J]==f[K])
                    g[I][K]++;
                }
    for(int L=0;L<=490;L++)
        for(int M=0;M<=3;M++)
            for(int O=0;O<=9;O++)
                {
                    if(e[L][M]==f[O])
                    h[L][O]++;
                }
    for(int P=0;P<=490;P++)
    {
        k=0;
        for(int Q=0;Q<=N-1;Q++)
            {   
                sum=0;
                for(int R=0;R<=9;R++)
                    {
                        y=min(g[Q][R],h[P][R]);//将数字个数小的那一个全部相加，与a数组对应的数比较，验证第一个条件
                        sum+=y;
                    }
                if(sum==a[Q])
                k++;
            }
        if(k==N)//条件全部满足时，使k加1并记录下来c中对应数
        {
            z++;
            number=c[P];
        }
    }
    if(z) printf("%d",number);//排除多解和无解情况
    else printf("Not sure");
    return 0;
}