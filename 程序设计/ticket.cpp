#include <stdio.h>
#include <string.h>
#include<cmath>
#include<iostream>
using namespace std;

int main() 
{
    struct move
    {
        int a;
        char first;
        char final;
    };
    int N;
    scanf("%d",&N);
    move all[100]={};
    int one[5]={},two[5]={},three[5]={};
    int mi1=50,ma2=0;
    for(int i=0;;i++)
    {
        scanf("%d",&all[i].a);
        if(all[i].a==0)break;
        scanf("%c%c",&all[i].first,&all[i].final);
        if(all[i].a==1)
        {
            if(all[i].final-all[i].first==3)three[0]++;
            if(all[i].final-all[i].first==1)
            {
                if(all[i].first=='A')one[0]++;
                if(all[i].first=='B')one[1]++;
                if(all[i].first=='C')one[2]++;
            }
            if(all[i].final-all[i].first==2)
            {
                if(all[i].first=='A')two[0]++;
                else two[1]++;
            }
        }
        if(all[i].a==2)
        {
            if(all[i].final-all[i].first==3)three[0]--;
            if(all[i].final-all[i].first==1)
            {
                if(all[i].first=='A')one[0]--;
                if(all[i].first=='B')one[1]--;
                else one[2]--;
            }
            if(all[i].final-all[i].first==2)
            {
                if(all[i].first=='A')two[0]--;
                else two[1]--;
            }
        }
    }
    mi1=min(mi1,one[0]);
    mi1=min(mi1,one[1]);
    mi1=min(mi1,one[2]);
    for(int j=0;j<=2;j++)
    one[j]-=mi1;
    while(one[0]>0&&two[1]>0)
    {
        ma2++;
        one[0]--;
        two[1]--;
    }
    while(one[2]>0&&two[0]>0)
    {
        ma2++;
        one[2]--;
        two[0]--;
    }
    int last;
    last=N-mi1-ma2-three[0]-one[0]-one[1]-one[2]-two[0]-two[1];
    printf("%d",last);
    return 0;
}