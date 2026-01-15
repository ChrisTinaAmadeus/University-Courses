#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int power(int n)
{
    int sum = 1;
    while (n > 0)
    {
        sum *= 10;
        n--;
    }
    return sum;
}
int main()
{
    int n[3] = {};
    scanf("%1d%1d%1d", &n[0], &n[1], &n[2]);
    int num = 0;
    for (int i = 0; i < 3; i++)
    {
        num += n[i] * power(2 - i);
    }
    int extra;
    extra = (num % 12 != 0);
    int danyuan = num / 12 + extra, ceng, room;
    if (extra)
    {
        num %= 12;
        extra = (num % 2 != 0);
        ceng = num / 2 + extra;
        room = !extra + 1;
    }
    else
    {
        ceng = 6;
        room = 2;
    }
    printf("%d-%d0%d", danyuan, ceng, room);
    return 0;
}