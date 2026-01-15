#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

char *checkSubstr(char s1[], char s2[])
{
    char *findp = s2+strlen(s2)-1;
    int l1 = strlen(s1), l2 = strlen(s2);
    for (int i = 0; i < l2 - l1+1;i++)
    {
        bool findjudge = true;
        int f[220] = {};
        for (int k = i; k < i + l1;k++)
        {
            bool judge = false;
            for (int j = 0; j < l1; j++)
            {
                if (f[j] != 1 && s1[j] == s2[k])
                {
                    f[j] = 1;
                    judge = true;
                    break;
                }
                
            }
            if(!judge)
            {
                findjudge = false;
                break;
            }
        }
        if(findjudge)
        {
            if(findp>s2+i)
                findp = s2 + i;
        }
    }
    if(findp!=s2+strlen(s2)-1)
    return findp;
    else
        return NULL;
}

int main()
{
    char s1[220], s2[220];
    char *pRes = NULL;

    scanf("%s%s", s1, s2);
    pRes = checkSubstr(s1, s2);

    if ( pRes == NULL )
        printf("false\n");
    else{
        pRes[ strlen(s1) ] = '\0';//记得补上这玩意
        printf("%s", pRes);
    }
    return 0;
}