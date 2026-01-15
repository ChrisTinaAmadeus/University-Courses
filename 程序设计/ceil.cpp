#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#define MAX_SIZE 1e17
#define pmax 30000
using namespace std;
long long N, m1, m2;
long long ceilall[10005];
bool judgecan = false;
long long minstarttime = MAX_SIZE;
int isprime[pmax], prime[pmax], primenum = 0;
struct component
{
    long long prime;
    long long num;
};
component Mnum[5000];
long long mcount = 0;
void set()
{
    for (int i = 2; i <= pmax; i++)
        prime[i] = 1;
    for (long long i = 2; i * i <= pmax; i++)
    {
        if (prime[i] != 0)
        {
            for (long long j = i * i; j <= pmax; j += i)
                prime[j] = 0;
        }
    }
    for (int i = 0; i < pmax; i++)
    {
        if (prime[i] != 0)
        {
            isprime[primenum] = i;
            primenum += 1;
        }
    }
    long long M1 = m1;
    for (long long i = 0; M1 > 1; i++)
    {
        if (M1 % isprime[i] == 0) // 从小到大把m1的素因数添加到数组
        {
            Mnum[mcount].prime = isprime[i];
            while (M1 % isprime[i] == 0)
            {
                M1 /= isprime[i];
                Mnum[mcount].num++;
            }
            Mnum[mcount].num *= m2;
            mcount++;
        }
    }
}
int main()
{
    scanf("%lld%lld%lld", &N, &m1, &m2);
    set();
    if (mcount == 0)
    {
        judgecan = true;
        minstarttime = 0;
    }
    for (int i = 0; i < N; i++)
    {
        scanf("%lld", &ceilall[i]);
    }
    for (int i = 0; i < N; i++)
    {
        component thisnum[5000] = {};
        long long T1 = ceilall[i], tcount = 0, tmcount = 0;
        long long thisstarttime = 1;
        for (long long j = 0; T1 > 1 && j < primenum; j++)
        {
            if (T1 % isprime[j] == 0)
            {
                thisnum[tcount].prime = isprime[j];
                while (T1 % isprime[j] == 0)
                {
                    T1 /= isprime[j];
                    thisnum[tcount].num++;
                }
                if (Mnum[tmcount].prime == thisnum[tcount].prime)
                {
                    while (thisstarttime * thisnum[tcount].num < Mnum[tmcount].num)
                    {
                        thisstarttime++;
                    }
                    tmcount++;
                }
                tcount++;
            }
        }
        if (tmcount != mcount)
            continue;
        minstarttime = min(minstarttime, thisstarttime);
        judgecan = true;
    }
    if (judgecan)
        printf("%lld", minstarttime);
    else
        printf("-1");
    return 0;
}