#include<stdio.h>
int main()
{
    int a,b,i;
    scanf("%d %d",&a,&b);
    if(a<b)
    {
        for(i=b;i<1e6;i++)
    {
        if(i%a==0&&i%b==0)
        {
        printf("%d",i);
        break;
        }
    }
    }
    else if(a>b)
    {
        for(i=a;i<1e6;i++)
        {
        if(i%a==0&&i%b==0)
        {printf("%d",i);
        break;
        }
        }
    }
    else if(a=b)
    {
        printf("%d",a);
    }
    return 0;
}