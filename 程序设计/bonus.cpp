#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
int main() 
{
    int n;
    scanf("%d",&n);
    struct standard
    {
        int line;
        double percent;
    };
    standard bonus[105]={};
    for(int i=1;i<=n;i++)
        scanf("%d%lf",&bonus[i].line,&bonus[i].percent);
    double sum=0;
    int benefit;
    scanf("%d",&benefit);
    if(benefit<0)printf("NO");
    else
    {
    for(int i=1;i<=n;i++)
    {
        if(bonus[i].line!=-1)
        {
            if(benefit>=bonus[i].line)sum+=(bonus[i].line-bonus[i-1].line)*bonus[i].percent;
            else 
            {
                sum+=bonus[i].percent*(benefit-bonus[i-1].line);
                break;
            }
        }
        else sum+=bonus[i].percent*(benefit-bonus[i-1].line);
    }
    printf("%.2lf",sum);
    }
    return 0;
}