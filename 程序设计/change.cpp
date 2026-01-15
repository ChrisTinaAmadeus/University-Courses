#include<stdio.h>
int main()
{
    char a;
    scanf("%c",&a);
    if(a>='a'&&a<='z')
    {
        a=a-'a'+'A';
    }
    else if(a>='A'&&a<='Z')
    {
        a=a-'A'+'a';
    }
    printf("%c",a);
    return 0;
}