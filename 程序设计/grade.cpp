#include<stdio.h>
int main()
{
    int a;
    scanf("%d",&a);
    if(a>=90&&a<=100)
    {
        printf("4.0");
    }
    else if(a>=86&&a<=89)
    {
        printf("3.7");
    }
    else if(a>=83&&a<=85)
    {
        printf("3.3");
    }
    else if(a>=80&&a<=82)
    {
        printf("3.0");
    }
    else if(a>=76&&a<=79)
    {
        printf("2.7");
    }
    else if(a>=73&&a<=75)
    {
        printf("2.3");
    }
    else if(a>=70&&a<=72)
    {
        printf("2.0");
    }
    else if(a>=66&&a<=69)
    {
        printf("1.7");
    }
    else if(a>=63&&a<=65)
    {
        printf("1.3");
    }
    else if(a>=60&&a<=62)
    {
        printf("1.0");
    }
    else if(a<60)
    {
        printf("0");
    }
    return 0;
}

// #include<stdio.h>
// int main()
// {
    // int a,b,c;
    // scanf("%d",&a);
    // b=a/10;
    // c=a%10;
    // float d,e;
    // if(b==9||b==10)
    // d=4.0;
    // else if(b==8)
    // d=3.0;
    // else if(b==7)
    // d=2.0;
    // else if(b==6)
    // d=1.0;
    // else if(b<6)
    // d=0;
    // if(c>=0&&c<=2)
    // e=0;
    // else if(c>=3&&c<=5)
    // e=0.3;
    // else if(c>=6&&c<=9)
    // e=0.7;
    // if(d==0||d==4.0)
    // e=0;
    // if(d==0&&e==0)
    // printf("%d",d+e);
    // else if(d!=0)
    // printf("%.1f",d+e);
    // return 0;
// }