#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    char name[60]={};
    int average,finalgrade,num;
    char position,west;
    scanf("%s %d%d%*c%c%*c%c%d",name,&average,&finalgrade,&position,&west,&num);
    int sum=0;
    if(average>80&&num>=1)sum+=6000;
    if(average>85&&finalgrade>80)sum+=5000;
    if(average>90)sum+=4000;
    if(average>85&&west=='Y')sum+=2500;
    if(position=='Y'&&finalgrade>80)sum+=950;
    printf("%s %d",name,sum);
    return 0;
}