#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
void merge(int a[], int low, int mid, int high, int b[])
{
    int i = low, j = mid + 1, k = 0;
    while ((i <= mid) && (j <= high))
    {
        if (a[i] <= a[j])
        b[k++] = a[i++];
        else
        b[k++] = a[j++];
    }
    while (i <= mid)
        b[k++] = a[i++];
    while (j <= high)
        b[k++] = a[j++];
    for (i = low, k = 0; i <= high; i++)
        a[i] = b[k++];
}
void sort(int a[], int low, int high, int b[])
{
    int mid;
    if (low < high)
    {
        mid = (low + high) / 2;
        sort(a, low, mid, b);
        sort(a, mid + 1, high, b);
        merge(a, low, mid, high, b);
    }
}

int main() 
{
    int n;
    scanf("%d",&n);
    int num1[10]={},num2[10]={};
    for(int i=0;i<n;i++)
    scanf("%d",&num1[i]);
    sort(num1,0,n-1,num2);
    for(int i=0;i<n;i++)
    printf("%d ",num1[i]);
    return 0;
}