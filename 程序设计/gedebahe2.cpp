#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int prime[1000005];
void isprime()
{
    for(int i=2;i<1000005;i++)
    {
        prime[i] = 1;
    }
    for(int i=2;i*i<1000005;i++)
    {
        if(prime[i]!=0)
        {
            for (int j = i * i; j < 1000005;j+=i)
                prime[j] = 0;
        }
    }
}
struct num
{
    int n;
    int min[5];
    int max[5];
    int ncount;
};
num all[250];
void statistic(int k)
{
    bool judge1 = false;
    for(int i=all[k].n-1;i>=all[k].n/2;i--)
    {
        if(prime[i]!=0&&prime[all[k].n-i]!=0)
        {
            if(!judge1)
            {
                all[k].max[0] = all[k].n - i;
                all[k].max[1] = i;
                judge1 = true;
            }
            all[k].min[0]=all[k].n-i;
            all[k].min[1] = i;
            all[k].ncount++;
        }
    }
}
int main()
{
    isprime();
    int n;
    scanf("%d",&n);
    for(int i=0;i<n;i++)
    {
        scanf("%d",&all[i].n);
        statistic(i);
    }
    for(int i=0;i<n;i++)
    {
        printf("%d+%d %d+%d %d\n", all[i].max[0], all[i].max[1], all[i].min[0], all[i].min[1], all[i].ncount);
    }
    return 0;
}