#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
enum who{A,B,C,D,E,F,G,H};
int main() 
{
    int n;
    scanf("%d",&n);
    int judge=0,thisman;
    for(int he=A;he<=H;he++)
    {
        int truecount;
        truecount=(he==H||he==F)+(he==B)+(he==G)+(he!=B)+(!((he==H||he==F)))+(he!=F&&he!=H)+(he!=C)+(he==H||he==F);
        if(truecount==n)
        {
            judge++;
            thisman=he;
        }
    }
    if(judge!=1)printf("DONTKNOW");
    else printf("%c",thisman+'A');
    return 0;
}