#include<stdio.h>
int main()
{
    float price,all;
    int num;
    scanf("%f%d",&price,&num);
    if(num<2)
    all=price*num;
    else if(num==2)
    all=price*num*0.8;
    else if(num>=3&&num<=5)
    all=price*num*0.7;
    else if(num>5)
    all=3.5*price+(num-5)*price*1.1;
    printf("%.2f",all);
    return 0;
}