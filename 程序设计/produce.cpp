#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
struct constant
{
    int ncount;
    int num[120];
};
constant all[20];
long long step = 1;
int p[120], f[120];
void pailie(int n, int m, constant a[], int k)
{
    if (k == n + 1)
    {
        printf("%d. ", step);
        for (int i = 1; i <= n; i++)
            printf("%d ", p[i]);
        printf("\n");
        step++;
        return;
    }
    else
    {
        for (int i = 1; i <= n; i++)
        {
            if (f[i] != 1)
            {
                bool judge1 = true;
                int J;
                for (int j = 0; j < m && judge1; j++) // 检验是否遇到固定工序
                {
                    if (a[j].num[0] == i)
                    {
                        judge1 = false;
                        J = j;
                        f[a[j].num[0]] = 1;
                        for (int t = 0; t < a[j].ncount; t++)
                        {
                            p[k] = a[j].num[t];
                            k++;
                        }
                    }
                }
                if (judge1) // 如果没有遇到固定工序
                {
                    f[i] = 1;
                    p[k] = i;
                    k += 1;
                }
                pailie(n, m, a, k);
                if (judge1)
                {
                    k -= 1;
                    f[i] = 0;
                    p[k] = 0;
                }
                else
                {
                    f[a[J].num[0]] = 0;
                    for (int t = 0; t < a[J].ncount; t++)
                    {
                        k -= 1;
                        p[k] = 0;
                    }
                }
            }
        }
    }
}
int main()
{
    int N, m;
    scanf("%d%d", &N, &m);
    for (int i = 0; i < m; i++)
    {
        scanf("%d", &all[i].ncount);
        for (int j = 0; j < all[i].ncount; j++)
        {
            scanf("%d", &all[i].num[j]);
            int t = all[i].num[j];
            if (j > 0)
                f[t] = 1;
        }
    }
    pailie(N, m, all, 1);
    return 0;
}