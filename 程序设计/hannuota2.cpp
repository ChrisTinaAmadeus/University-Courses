#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
void move(int n,char a,char c,char b)
{
    if(n==1)
    {
        printf("%d %c %c\n", n, a, b);
        return;
    }
    move(n - 1, a, b, c);
    printf("%d %c %c\n", n , a, b);
    move(n - 1, c, a, b);
}
int main()
{
    int n;
    scanf("%d", &n);
    char A = 'A', B = 'B', C = 'C';
    move(n,A,C,B);
    return 0;
}