#include <stdio.h>
int main()
{
    double s=0.0;
    int n;
    int k;
    scanf("%d",&k);
    for(n=1;s<=k;n++) 
    s+=1.0/n;
    printf("%d",n-1);
    return 0;
}