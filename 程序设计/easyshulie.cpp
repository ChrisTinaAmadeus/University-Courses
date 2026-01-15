#include<stdio.h>
#include<iostream>
using namespace std;
#include<math.h>
int main()
{
    int sum,a,b,c,d,e;
    scanf("%d",&sum);
    int Q[5];
    for(a=1;a<=5;a++)
        for(b=1;b<=5;b++)
            for(c=1;c<=5;c++)
                for(d=1;d<=5;d++)
                    for(e=1;e<=5;e++)
                    {
                        if(a+4*b+6*c+4*d+e==sum)
                            if(a!=b&&a!=c&&a!=d&&a!=e&&b!=c&&b!=d&&b!=e&&c!=d&&c!=e&&d!=e)
                            {
                                printf("%d %d %d %d %d",a,b,c,d,e);
                                return 0;
                            }
                    }

    return 0;
}