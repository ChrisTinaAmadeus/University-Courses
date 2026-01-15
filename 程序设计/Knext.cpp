#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
using namespace std;
struct sample
{
    int type;
    int n;
    int wei[10005][2];
};
sample all[105];
struct disandtype
{
    int dis;
    int type;
};
disandtype result[10005];
int distance(int a[], int b[])
{
    int x = (a[0] - b[0]) * (a[0] - b[0]);
    int y = (a[1] - b[1]) * (a[1] - b[1]);
    int sum = int(x + y);
    return sum;
}
int partition(int l, int r)
{
    disandtype k = result[l];
    while(l!=r)
    {
        while((l<r)&&((result[r].dis>k.dis)||(result[r].dis==k.dis&&result[r].type>k.type)))
            r--;
        if(l<r)
        {
            result[l]=result[r];
            l += 1;
        }
        while ((l < r) && ((result[l].dis < k.dis) || (result[l].dis == k.dis && result[l].type < k.type)))
            l++;
        if (l < r)
        {
            result[r] = result[l];
            r -= 1;
        }
        
    }
    result[l] = k;
        return l;
}
void quicksort(int l, int r)
{
    if (l >= r)
        return;
    int mid = partition(l, r);
    quicksort(l, mid - 1);
    quicksort(mid + 1, r);
}
int main()
{
    int m, k;
    scanf("%d%d", &m, &k);
    for (int i = 1; i <= m; i++)
    {
        all[i].type = i;
        scanf("%d", &all[i].n);
        for (int j = 1; j <= all[i].n; j++)
        {
            for (int k = 0; k <= 1; k++)
            {
                scanf("%d", &all[i].wei[j][k]);
            }
        }
    }
    int s[2] = {};
    scanf("%d%d", &s[0], &s[1]);
    int ncount = 1;
    for (int i = 1; i <= m; i++)
    {
        for (int j = 1; j <= all[i].n; j++)
        {
            result[ncount].dis = distance(s, all[i].wei[j]);
            result[ncount].type = i;
            ncount += 1;
        }
    }
    quicksort(1, ncount - 1);
    int finaltype = 0;
    int kcount[110] = {};
    for (int i = 1; i <= 10001;i++)
    {
        if(i<=k)
        {
            kcount[result[i].type]++;
        }
        else
        {
            if(result[i].dis==result[k].dis)
                kcount[result[i].type]++;
            else
                break;
        }
    }
    int ma=0;
    for (int i = 1; i <= m;i++)
    {
        if(max(kcount[i],ma)!=ma)
        {
            finaltype = i;
            ma = kcount[i];
        }
    }
    printf("%d", finaltype);
    return 0;
}