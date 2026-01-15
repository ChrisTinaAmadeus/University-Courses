#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#define MAX 3000000000
using namespace std;
struct num
{
    bool judge=false;
    int hang;
    int lie;
};
int main()
{
    num *p = (num *)malloc(MAX * sizeof(num));
    memset(p, 0, MAX);
    int n;
    scanf("%d", &n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            int wei;
            scanf("%d", &wei);
            p[wei].judge = true;
            p[wei].hang = i;
            p[wei].lie = j;
        }
    int k;
    scanf("%d", &k);
    for (int i = 0; i < k;i++)
    {
        int K;
        scanf("%d", &K);
        if(p[K].judge)
            printf("%d %d\n", p[K].hang, p[K].lie);
        else
            printf("-1\n");
    }
    free(p);
    return 0;
}