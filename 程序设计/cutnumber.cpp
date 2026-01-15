#include<iostream>
using namespace std;
#include<stdio.h>
#include<math.h>
int main()
{
    int a,i;
    scanf("%d",&a);
    int ge,shi,bai,qian,wei,k=1;
    for(qian=0;qian<=9&&k;qian++)//最后这些数都会比实际大1
        for(bai=0;bai<=9&&k;bai++)
            for(shi=0;shi<=9&&k;shi++)
                for(ge=0;ge<=9&&k;ge++)
                {
                    if(1000*qian+100*bai+10*shi+ge==a)
                    {
                        k=0;
                    }
                }
    printf("%d,",ge-1);
    wei=1;
    int b[3]={shi-1,bai-1,qian-1};
    for(i=0;i<=2;i++)
    {
        if(b[2]!=0)
        {
            printf("%d,",b[i]);
            wei=4;
        }
        else
        {
            if(b[1]!=0)
            {
            printf("%d,%d,",b[0],b[1]);
            wei=3;
            break;
            }
            else
            {
                if(b[0]!=0)
                {
                    printf("%d,",b[0]);
                    wei=2;
                    break;
                }
            }
        }
    }
    printf("%d",wei);
    return 0;
}