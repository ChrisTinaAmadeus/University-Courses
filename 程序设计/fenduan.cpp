#include<stdio.h>
#include<math.h>
int main()
{
    float a;
    scanf("%f",&a);
    if(a<0)
    {
        printf("%.2f",-a);
    }
    else if(a>=0&&a<=1)
    {
        printf("%.2f",sqrt(a));
    }
    else if(a>1)
    {
        printf("%.2f",a*a);
    }
    return 0;
}