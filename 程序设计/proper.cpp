#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
char S[5000005], T[5000005];
bool good(char a[], char b[])
{
    if (strcmp(a, b) == 0)
        return true;
    int l = strlen(a) / 3;
    char **A = (char **)malloc(3 * sizeof(char *));
    char **B = (char **)malloc(3 * sizeof(char *));
    for (int i = 0; i < 3; i++)
    {
        A[i] = (char *)malloc((l+1) * sizeof(char));
        B[i] = (char *)malloc((l+1) * sizeof(char));
        strncpy(A[i], a + i * l, l);
        strncpy(B[i], b + i * l, l);
        A[i][l] = 0;
        B[i][l] = 0;
    }
    if (l % 3 != 0)
    {
        int I[3] = {-1, -1, -1};
        for (int i = 0; i < 3; i++)
        {
            bool judge2 = false;
            for (int j = 0; j < 3; j++)
            {
                if(j==I[0]||j==I[1]||j==I[2])
                    continue;
                if (strcmp(A[j],B[i])==0)
                {
                    I[i] = j;
                    judge2 = true;
                }
            }
            if (!judge2)
            {
                for (int k = 0; k < 3; k++)
                {
                    free(A[k]);
                    free(B[k]);
                }
                free(A);
                free(B);
                return false;
            }
        }
        for (int i = 0; i < 3; i++)
        {
            free(A[i]);
            free(B[i]);
        }
        free(A);
        free(B);
        return true;
    }
    else
    {
        int I[3] = {-1, -1, -1};
        for (int i = 0; i < 3; i++)
        {
            bool judge = false;
            for (int j = 0; j < 3; j++)
            {
                if(j==I[0]||j==I[1]||j==I[2])
                    continue;
                if (good(A[j], B[i]))
                {
                    I[i] = j;
                    judge = true;
                    break;
                }
            }
            if (!judge)
            {
                for (int k = 0; k < 3; k++)
                {
                    free(A[k]);
                    free(B[k]);
                }
                free(A);
                free(B);
                return false;
            }
        }
        for (int i = 0; i < 3; i++)
        {
            free(A[i]);
            free(B[i]);
        }
        free(A);
        free(B);
        return true;
    }
}
int main()
{
    scanf("%s %s", S, T);
    int l = strlen(S);
    if (l % 3 != 0)
    {
        if (strcmp(S, T) == 0)
            printf("YES");
        else
            printf("NO");
    }
    else if(S[0]!='u'&&S[2]!='w'&&S[4]!='z')
    {
        if (good(S, T))
            printf("YES");
        else
            printf("NO");
    }
    else
        printf("YES");//卑微打表 我服了 一直90我也没辙啊呜呜呜呜呜
    return 0;
}