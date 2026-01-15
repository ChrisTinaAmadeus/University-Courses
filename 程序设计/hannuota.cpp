#include <stdio.h>//汉诺塔
#include <string.h>
#include<iostream>
using namespace std;
long long steps=0;
void move(int n,char p,char q,char r)
{
    if(n==1)
    {
        steps++;
        printf("[step %lld] move plate %d# from %c to %c\n",steps,n,p,r);
    }
    else
    {
        move(n-1,p,r,q);
        steps++;
        printf("[step %lld] move plate %d# from %c to %c\n",steps,n,p,r);
        move(n-1,q,p,r);
    }
}
int main() 
{
    int N;
    scanf("%d",&N);
    char A='a',B='b',C='c';
    move(N,A,B,C);
    printf("%lld",steps);
    return 0;
}