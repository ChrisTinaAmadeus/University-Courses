#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char num[20] = {};
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
bool judgenum(int l, int r)
{
    if (num[l] - '0' == 0 && r > l)
        return false;
    int sum = 0;
    for (int i = l; i <= r; i++)
    {
        sum += (num[i] - '0') * power(r - i);
    }
    if (sum >= 0 && sum <= 255)
        return true;
    return false;
}
void print(int l, int r)
{
    for (int i = l; i <= r; i++)
        printf("%c", num[i]);
}
int main()
{
    scanf("%s", num);
    int l = strlen(num);
    for (int i = 0; i < l - 2; i++)
        for (int j = i + 1; j < l - 1; j++)
            for (int k = j + 1; k < l; k++)
            {
                if (judgenum(0, i) && judgenum(i + 1, j) && judgenum(j + 1, k)&&judgenum(k+1,l-1))
                {
                    print(0, i);
                    printf(".");
                    print(i + 1, j);
                    printf(".");
                    print(j + 1, k);
                    printf(".");
                    print(k + 1, l - 1);
                    printf("\n");
                }
            }
    return 0;
}