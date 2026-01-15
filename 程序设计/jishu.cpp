#include<stdio.h>
#include<math.h>
int main()
{
    int a,b,n,k,m,c,d,e=0;
    int p,q;
    scanf("%d%d%d%d%d",&a,&b,&n,&k,&m);
    p=pow(10,m-1);
    q=pow(10,m);
    for(c=a;c<=b;c++)
    {
        if(c>=p&&c<q)
        {
            if(c%10==n)
            {
                if(c%k==0)
                    {
                    e=e+1;
                    }
            }
        }
    }
    printf("%d",e);
    return 0;
}