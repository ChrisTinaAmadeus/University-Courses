#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int n;
    scanf("%d",&n);
    int a,b,way=0,e=1,f=1;
    for(a=0;a<=30;a++)
        for(b=0;b<=15;b++)
        {
            if(a+2*b==n)
            {
                if(a==0||b==0)
                way++;
                else
                {
                    int c,g;
                    c=a+b;
                    g=min(a,b);
                    for(c;c>=a+b-g+1;c--)
                    {
                        e*=c;
                    }
                    int d;
                    d=g;
                    for(d;d>=1;d--)
                    {
                        f*=d;
                    }
                    way+=e/f;
                    e=1;
                    f=1;
                }
            }
        }
    printf("%d",way);
    return 0;
}