#include<stdio.h>
#include<iostream>
#include<math.h>
using namespace std;
int main()
{
    int i,j,k,n;
    char a;
    scanf("%d",&n);
    for(i=1;i<=n;i++){
        for(k=1;k<=n-i;k++){
            printf(" ");
        }
        for(j=1;j<=i;j++){
            a='A'+j-1;
            printf("%c",a);
        }
        for(j=i;j>1;j--){
            a='A'+j-2;
            printf("%c",a);
        }
        printf("\n");
    }
return 0;
}