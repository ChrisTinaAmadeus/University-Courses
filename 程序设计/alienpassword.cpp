#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char initial[50000];
int step;
void dfs(int n)
{
    if(initial[n+2]>='0'&&initial[n+2]<='9')
    {
        int s = (initial[n + 1] - '0') * 10 + initial[n + 2] - '0';
        for (int i = 1; i <= s; i++)
        {
            for (int j = n + 3;; j++)
            {
                if (initial[j] == '[')
                {
                    dfs(j);
                    j = step;
                }
                else if (initial[j] == ']')
                {
                    step = j;
                    break;
                }
                else if (initial[j] != ']' && initial[j] != '[')
                    printf("%c", initial[j]);
            }
        }
    }
    else
    {
        int s = initial[n + 1] - '0';
        for (int i = 1; i <= s;i++)
        {
            for (int j = n + 2;;j++)
            {
                if(initial[j]=='[')
                {
                    dfs(j);
                    j = step;
                }
                else if(initial[j]==']')
                {
                    step = j;
                    break;
                }
                else if (initial[j] != ']' && initial[j] != '[')
                    printf("%c", initial[j]);
            }
        }
    }
}
int main()
{
    scanf("%s", initial);
    for (int i = 0; i < strlen(initial);)
    {
        if(initial[i]!='[')
        {
            printf("%c", initial[i]);
            i += 1;
        }
        else
        {
            dfs(i);
            int ncount = 1;
            for (int j = i+1;;j++)
            {
                if(initial[j]=='[')
                    ncount += 1;
                if(initial[j]==']')
                    ncount -= 1;
                if(ncount==0)
                {
                    i = j+1;
                    break;
                }
            }
        }
    }
    return 0;
}