#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    char sentence[20][400]={};
    char initial[400]={};
    gets(initial);
    int initialc[30]={};
    for(int i=0;i<strlen(initial);i++)
    {
        if(initial[i]>='A'&&initial[i]<='Z')
        initial[i]+=('a'-'A');
        if(initial[i]>='a'&&initial[i]<='z')
        initialc[initial[i]-'a']++;
    }
    int n;
    scanf("%d",&n);
    getchar();
    bool findone=false;
    for(int i=1;i<=n;i++)
    {
        gets(sentence[i]);
        int sc[30] = {};
        for (int j = 0; j < strlen(sentence[i]); j++)
        {
            if (sentence[i][j] >= 'A' && sentence[i][j] <= 'Z')
                sentence[i][j] += ('a' - 'A');
            if (sentence[i][j] >='a'&&sentence[i][j]<='z')
                sc[sentence[i][j] - 'a']++;
        }
        bool judge=true;
        for(int j=0;j<26&&judge;j++)
        {
            if(sc[j]!=initialc[j])
            judge=false;
        }
        if(judge)
        {
            findone=true;
            printf("%d ",i);
        }
    }
    if(!findone)printf("0");
    return 0;
}