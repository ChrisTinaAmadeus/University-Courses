#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
long long power(int n)
{
    long long sum=1;
    while(n>0)
    {
        sum*=10;
        n--;
    }
    return sum;
}
int main()
{
    char str[1100] = {};
    gets(str);
    long long ncount = 0;
    for (int i = 0; i < strlen(str);)
    {
        if(str[i]>='0'&&str[i]<='9')
        {
            int length=1;
            for (int j = i + 1;;j++)
            {
                if(str[j]>='0'&&str[j]<='9')
                {
                    length++;
                }
                else
                    break;
            }
            int l = length;
            for(int j=i;j<i+l;j++)
            {
                ncount+=((str[j]-'0')*power(length-1));
                length--;
            }
            i += l;
        }
        else
            i++;
    }
    printf("%lld", ncount);
    return 0;
}