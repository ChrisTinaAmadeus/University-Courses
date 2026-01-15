#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
using namespace std;
int N;
int dian[500][2];
int fabsdian(int n)
{
    if (n < 0)
        return (-n);
    else
        return n;
}
bool kjudge(int kx, int ky, int x1, int y1, int x2, int y2)
{
    int mx = x1 - x2;
    int my = y1 - y2;
    if (ky*mx!=kx*my)//化除法为乘法，多用数学思想
        return false;
    return true;
}
int main()
{
    scanf("%d", &N);
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j <= 1; j++)
        {
            scanf("%d", &dian[i][j]);
        }
    }
    int S0 = 0, S1 = 1;
    int mncount = 0;
    int ncount = 0;
    for (int i = 0; i < N - 1; i++)
    {
        for (int j = i + 1; j < N; j++)
        {
            ncount = 0;
            for (int k = 0; k < N; k++)
            {
                if (k == i || k == j)
                    continue;
                if (kjudge(dian[i][0] - dian[j][0], dian[i][1] - dian[j][1], dian[i][0], dian[i][1], dian[k][0], dian[k][1]))
                {
                    ncount += 1;
                }
            }
            if (max(ncount, mncount) != mncount)
            {
                mncount = ncount;
                S0 = i;
                S1 = j;
            }
        }
    }
    printf("%d %d", S0, S1);
    return 0;
}