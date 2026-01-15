#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
    int a,b,c,m=0;
    float N;
    scanf("%f",&N);
    for(a=1;a<=100;a++)
        for(b=1;b<=16;b++)
            for(c=8;c<=100;c+=10)
            {
                if(a!=b&&a!=c&&b!=c)
                {
                    if(a>min(b,c)&&a<max(b,c))
                    {
                        if(a%2==0)
                        {
                            if(2*a+6*b+c>0.9*N&&2*a+6*b+c<=N)
                            {
                                if(b<10&&a>=10)
                                {
                                    printf("%d %d %d\n",a,b,c);
                                    m=1;
                                }
                                else if(b>=10)
                                {
                                    printf("%d %d %d\n",a,b,c);
                                    m=1;
                                }
                            }
                        }
                    }
                }
            }
if(!m)
printf("no answer");
return 0;
}