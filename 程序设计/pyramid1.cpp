#include<stdio.h>
int main()
{
    int n,m,t,c;
    scanf("%d",&n);
    m=n+'A'-1;
    t='A';
    c=n-1;
    int i,j,f,p;
    for(i=1;i<=n;i++)
    {
        for(p=1;p<=c;p++)
            {
                printf(" ");
            }
        c--;
        for(j=65;j<=t;j++)
        {
            
            printf("%c",j);
        }
        for(f=t-1;f>=65;f--)
        {
            printf("%c",f);
        }
        printf("\n");
        t++;
        t<=m;
    }
    return 0;
}