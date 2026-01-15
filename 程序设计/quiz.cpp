#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int n;
int score[500];
int getscore[500];
int giveup, falsenum;
int scorenow;
void countall(int sno)
{
    if (sno == 19||scorenow>=120||falsenum==3)//终止条件最好放一起
    {
        score[scorenow]++;
        return;
    }


    int recordfalse = falsenum;
    falsenum = 0;
    scorenow += 10;
    countall(sno + 1);
    scorenow -= 10;
    falsenum = recordfalse;


    int recordscore = scorenow;
    if (scorenow <= 30)
    {
        scorenow = 0;
        falsenum++;
        countall(sno + 1);
        scorenow = recordscore;
        falsenum--;
    }
    else
    {
        int minus = (scorenow * (18 - sno)) / 36;
        scorenow -= minus;
        falsenum++;
        countall(sno + 1);
        scorenow += minus;
        falsenum--;
    }

    if (giveup < 3)
    {
        giveup += 1;
        countall(sno + 1);
        giveup -= 1;
    }
}
int main()
{
    scanf("%d", &n);
    for (int i = 0; i < n; i++)
    {
        scanf("%d", &getscore[i]);
    }
    countall(1);
    for (int i = 0; i < n; i++)
        printf("%d ", score[getscore[i]]);
    return 0;
}