#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
const char order[5] = {"RUC"};
char all[50];
long long ncount;
int n;
void dfs(int k)
{
    if (k == n)
    {
        // printf("%s\n", all);
        ncount++;
        return;
    }
    for (int i = 0; i <= 2; i++)
    {
        if (k == 0)
        {
            all[k] = order[i];
            dfs(k + 1);
            all[k] = '\0';
        }
        if (k > 0 && order[i] != all[k - 1] && (k != n - 1 || (k == n - 1 && order[i] != all[0])))
        {
            all[k] = order[i];
            dfs(k + 1);
            all[k] = '\0';
        }
    }
}
int main()
{
    scanf("%d", &n);
    dfs(0);
    printf("%lld", ncount);
    return 0;
}