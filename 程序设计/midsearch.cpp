#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

int bisearch( int ary[], int left, int right, int num )

{

    printf("search %d from %d to %d", num, left, right );
    cout << endl;
    int judge=-1;
    if(left>right)return judge;
    int mid=(right+left)/2;
    if(num<ary[mid]) judge=bisearch(ary,left,mid-1,num);
    if(num>ary[mid]) judge=bisearch(ary,mid+1,right,num);
    if(num==ary[mid]) return mid;
    return judge;
}

int main()
{
    int n, array[10000], i, num;
    scanf("%d", &n);
    scanf("%d", &num);
    for (i = 0; i < n; i ++ )
    {
        scanf("%d", &array[i]);
    }

    printf( "%d", bisearch(array, 0, n-1, num));
    cout << endl;
    return 0;
}    

