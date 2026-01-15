#include<stdio.h>
#include<math.h>
int main()
{
    float x;
    scanf("%f",&x);
    if(x<0)x=x*(-1.0);
    else if(x>=0&&x<=1)x=sqrt(x);
    else if(x>1)x=x*x;
    printf("%.2f",x);
    return 0;
}