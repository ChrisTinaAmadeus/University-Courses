#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
void bingwei(int mis[],int dmi,int d)
{
    int plus1 = d - dmi;
    for (int i = dmi - 1; i >= 0; i--)
    {
        mis[i + plus1] = mis[i];
        mis[i] = 0;
    }
}
void minus1(int r[],int mas[],int mis[])
{
    bool judgestart = false;
    int borrow = 0;
    for (int i = 199;i>=0;i--)
    {
        if(mas[i]!=0||mis[i]!=0)
            judgestart = true;
        if(judgestart)
        {
            mas[i] -= borrow;
            borrow = 0;
            if(mas[i]<0)
            {
                mas[i] += 10;
                borrow = 1;
            }
            r[i] = mas[i] - mis[i];
            if(r[i]<0)
            {
                r[i]+=10;
                borrow = 1;
            }
        }
    }
}
int main()
{
    int n;
    scanf("%d", &n);
    for (int j = 0; j < n; j++)
    {
        //printf("\n");
        char S1[200] = {}, S2[200] = {};
        scanf("%s %s", S1, S2);
        bool judge1dian = false, judge2dian = false;
        int dian1 = 0, dian2 = 0;
        int s1[200] = {}, s2[200] = {}, result[200] = {};
        for (int i = 0;; i++)
        {
            if (judge1dian && judge2dian)
                break;
            if (S1[i] == '.')
            {
                dian1 = i;
                judge1dian = true;
            }
            if(!judge1dian)
            {
                s1[i] = S1[i] - '0';
            }
            if (S2[i] == '.')
            {
                dian2 = i;
                judge2dian = true;
            }
            if(!judge2dian)
            {
                s2[i] = S2[i] - '0';
            }
        }
        if (dian1 > dian2)
            bingwei(s2, dian2,dian1);
        if (dian1 < dian2)
            bingwei(s1, dian1,dian2);
        bool judge1end = false, judge2end = false;
        int xstart = max(dian1, dian2);
        int S1end = 0, S2end = 0;
        for (int i = 1;;i++)
        {
            if(judge1end&&judge2end)
                break;
            if(S1[i+dian1]=='\0')
            {
                S1end = i+dian1-2;
                judge1end = true;
            }
            if(!judge1end)
            {
                s1[xstart+i-1] = S1[i+dian1] - '0';
            }
            if(S2[i+dian2]=='\0')
            {
                S2end = i+dian2-2;
                judge2end = true;
            }
            if(!judge2end)
            {
                s2[xstart+i - 1] = S2[i+dian2] - '0';
            }
        }
        if(dian1>=dian2)
        {
            if(dian1>dian2)
            {
                minus1(result,s1,s2);
            }
            else
            {
                for (int i = 0; i < dian1;i++)
                {
                    if(s1[i]!=s2[i])
                    {
                        if(s1[i]>s2[i])
                            minus1(result, s1, s2);
                        else
                        {
                            printf("-");
                            minus1(result, s2, s1);
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            printf("-");
            minus1(result, s2, s1);
        }
        bool zeroout = false;
        for (int i = 0;i<=max(S1end,S2end);i++)
        {
            if(result[i]!=0&&!zeroout)
                zeroout = true;
            if(i==max(dian1,dian2))
                printf(".");
            if(zeroout)
            printf("%d", result[i]);
        }
        printf("\n");
    }
    return 0;
}