#include<stdio.h>
#include<iostream>
using namespace std;
int three(int a)
{
    int b;
    b=a*a*a;
    return b;
}
int main()
{
    int n,m;
    scanf("%d%d",&n,&m);
    for(int i=n;i<=m;i++)
    {
        int bai,shi,ge;
        ge=i%10;
        shi=(i/10)%10;
        bai=(i/100)%10;
        if(i==three(ge)+three(bai)+three(shi))
        printf("%d ",i);
    }
    return 0;
}