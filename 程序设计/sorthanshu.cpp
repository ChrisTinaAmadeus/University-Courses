#include <stdio.h>
#include <stdlib.h>

void mySort(int *p,int l,int r)
{
    int temp;
    for (int i = l;i<=r;i++)
    {
        for (int j = l;j<=r-1;j++)
        {
            if(p[j]>p[j+1])
            {
                temp=p[j];
                p[j]=p[j+1];
                p[j + 1] = temp;
            }
        }
    }
}

int main()  
{  
    int array[100], i, n;
    
    scanf("%d",&n);

    for ( i = 0; i < n; i ++ ){
        scanf("%d", &array[i]);
    }

    mySort(array, 0, n - 1);

    for ( i = 0; i < n; i ++ ){
        printf("%d ", array[i]);
    }
    printf("\n");  
    return 0;  
}  