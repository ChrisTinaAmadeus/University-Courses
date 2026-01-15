#include <stdio.h>
#include <stdlib.h>

int findMax(int *p, int nSize)
{
    int maxnum = 0,temp,findi;
    for(int i=0;i<nSize;i++)
    {
        temp = maxnum;
        maxnum = maxnum < p[i] ? p[i] : maxnum;
        if(temp!=maxnum)
        {
            findi = i;
        }
    }
    temp = p[findi];
    p[findi]=p[0];
    p[0] = temp;
    return maxnum;
}

int main()
{
    int ary[2100] = {3, 2, 1, 5, 6, 7, 9, 10}, n = 8, nMax, i;
    scanf("%d", &n);
    for (i = 0; i < n; i++)
        scanf("%d", &ary[i]);

    nMax = findMax(ary, n);

    for (i = 0; i < n; i++)
        printf("%d ", ary[i]);
    printf("\n");

    printf("%d\n", nMax);
    return 0;
}