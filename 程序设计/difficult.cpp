#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
long long num[100005];
int partition(int l,int r)
{
    long long k = num[l];
    while(l!=r)
    {
        while((l<r)&&(num[r]>k))
            r--;
        if(l<r)
        {
            num[l]=num[r];
            l++;
        }
        while((l<r)&&(num[l]<=k))
            l++;
        if(l<r)
        {
            num[r]=num[l];
            r--;
        }
    }
    num[l] = k;
    return l;
}
void quicksort(int l,int r)
{
    if(l>=r)
        return;
    int mid = partition(l, r);
    quicksort(l, mid - 1);
    quicksort(mid + 1, r);
}
int main()
{
    int n;
    scanf("%d", &n);
    for (int i = 0; i < n;i++)
    {
        scanf("%d", &num[i]);
    }
    quicksort(0, n - 1);
    long long sum = 0;
    for (int i = 0; i < n;i+=2)
    {
        sum += (num[i + 1] - num[i]);
    }
    printf("%lld", sum);
    return 0;
}