#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    typedef struct
    {
        char name[100]={};
        int price;
        int num;
    }pack;
    pack item[100]={};
    int N;
    scanf("%d",&N);
    for(int i=0;i<N;i++)
        scanf("%s%*c%d%d",item[i].name,&item[i].price,&item[i].num);
    int sum=0;
    for(int i=0;i<N;i++)
        sum+=(item[i].price*item[i].num);
    printf("%d",sum);
    return 0;
}