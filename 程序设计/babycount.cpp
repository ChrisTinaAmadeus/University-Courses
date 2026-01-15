#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
int year[3000];
struct babies
{
    int ID;
    int birthday[5];
    int gender;
};
babies allbaby[10010];
void yearset()
{
    for (int i = 1969; i <= 2019; i++)
    {
        if ((i % 4 == 0 && i % 100 != 0) || i % 400 == 0)
            year[i] = 1;
    }
}
int month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct babycount
{
    int month1, month2;
    int day1, day2;
    int boynum;
    int allnum;
};
babycount all[100];
int week[] = {0, 7, 1, 2, 3, 4, 5, 6};
int w = 2;
void set1_1(int n)
{
    int sum = 0;
    for (int i = 1969; i < n; i++)
    {
        if (year[i] == 1)
            sum += 366;
        else
            sum += 365;
    }
    sum -= 3;
    sum %= 7;
    all[1].month1 = 1;
    all[1].day1 = 1;
    all[1].month2 = 1;
    if (sum != 0)
        all[1].day2 = 1 + 7 - sum;
    else
        all[1].day2 = 1;
    int daycount = all[1].day2;
    int thisyearday;
    if (year[n] == 1)
    {
        thisyearday = 366;
        month[2] += 1;
    }
    else
        thisyearday = 365;

    while (daycount < thisyearday)
    {
        if (daycount + 7 > thisyearday)
        {
            all[w].day1 = all[w - 1].day2 + 1;
            all[w].month1 = all[w - 1].month2;
            all[w].month2 = 12;
            all[w].day2 = 31;
            break;
        }
        if (all[w - 1].day2 + 1 <= month[all[w - 1].month2])
        {
            // 上周末尾不是一个月最后一天
            all[w].day1 = all[w - 1].day2 + 1;
            all[w].month1 = all[w - 1].month2;
            if (all[w].day1 + 6 <= month[all[w].month1])
            {
                all[w].day2 = all[w].day1 + 6;
                all[w].month2 = all[w].month1;
            }
            else
            {
                all[w].day2 = all[w].day1 + 6 - month[all[w].month1];
                all[w].month2 = all[w].month1 + 1;
            }
        }
        else
        {
            // 上周末尾是一个月最后一天
            all[w].day1 = 1;
            all[w].month1 = all[w - 1].month2 + 1;
            all[w].day2 = 7;
            all[w].month2 = all[w].month1;
        }
        w++;
        daycount += 7;
    }
}
int partition(int l, int r)
{
    babycount k = all[l];
    while (l != r)
    {
        while ((l < r) && (all[r].allnum < k.allnum || (all[r].allnum == k.allnum && all[r].month1 > k.month1) || (all[r].allnum == k.allnum && all[r].month1 == k.month1 && all[r].day1 > k.day1)))
            r--;
        if (l < r)
        {
            all[l] = all[r];
            l += 1;
        }
        while ((l < r) && (all[l].allnum > k.allnum || (all[l].allnum == k.allnum && all[l].month1 < k.month1) || (all[l].allnum == k.allnum && all[l].month1 == k.month1 && all[l].day1 < k.day1)))
            l++;
        if (l < r)
        {
            all[r] = all[l];
            r -= 1;
        }
    }
    all[l] = k;
    return l;
}
void quicksort(int l, int r)
{
    if (l >= r)
        return;
    int mid = partition(l, r);
    quicksort(l, mid - 1);
    quicksort(mid + 1, r);
}
int main()
{
    yearset();
    int thisyear, n, m;
    scanf("%d%d%d", &thisyear, &n, &m);
    set1_1(thisyear);
    for (int i = 0; i < n; i++)
    {
        scanf("%d%d%d%d", &allbaby[i].ID, &allbaby[i].birthday[0], &allbaby[i].birthday[1], &allbaby[i].gender);
        int allday = 0;
        for (int j = 1; j < allbaby[i].birthday[0]; j++)
            allday += month[j];
        allday += allbaby[i].birthday[1];
        if (allday <= all[1].day2)
        {
            all[1].allnum++;
            if (allbaby[i].gender == 1)
                all[1].boynum++;
        }
        else
        {
            int extra = ((allday - all[1].day2) % 7 != 0);
            int week = (allday - all[1].day2) / 7 + 1;
            all[week + extra].allnum++;
            if (allbaby[i].gender == 1)
                all[week + extra].boynum++;
        }
    }
    quicksort(1, w);
    for (int i = 1; i <= m; i++)
    {
        printf("[%2d.%2d-%2d.%2d]:%d(B%d)\n", all[i].month1, all[i].day1, all[i].month2, all[i].day2, all[i].allnum, all[i].boynum);
    }
    return 0;
}