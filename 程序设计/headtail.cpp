#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
char all[100100][210];
int main()
{
    int n;
    scanf("%d",&n);
    char order[10];
    int m;
    scanf("%s %d",order,&m);
    getchar();
    for(int i=0;i<n;i++)
    gets(all[i]);
    if(order[0]=='h')
    {
        for(int i=0;i<m;i++)
        printf("%s\n",all[i]);
    }
    else
    {
        for(int i=n-m;i<n;i++)
        printf("%s\n",all[i]);
    }
    return 0;
}