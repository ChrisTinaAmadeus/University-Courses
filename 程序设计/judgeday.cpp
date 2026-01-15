#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    int n;
    scanf("%d", &n);
    if (n % 7 == 1)
        printf("SUN");
    if (n % 7 == 2)
        printf("MON");
    if (n % 7 == 3)
        printf("TUE");
    if (n % 7 == 4)
        printf("WED");
    if (n % 7 == 5)
        printf("THU");
    if (n % 7 == 6)
        printf("FRI");
    if (n % 7 == 0)
        printf("SAT");
    return 0;
}