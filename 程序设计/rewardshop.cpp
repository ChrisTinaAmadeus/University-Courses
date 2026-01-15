#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;

int main() 
{
    int a,b,c,d;
    scanf("%d%d%d%d",&a,&b,&c,&d);
    int gai=d/a,ping=d/a;
    int ncount=d/a;
    while(ping>=b||gai>=c)
    {
        while(ping>=b)
        {
            ping-=b;
            ncount++;
            ping++;
            gai++;
        }
        while(gai>=c)
        {
            gai-=c;
            ncount++;
            ping++;
            gai++;
        }
    }
    printf("%d",ncount);
    return 0;
}