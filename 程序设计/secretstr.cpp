#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

int main()
{
    char str[50] = {};
    int k;
    scanf("%s %d",str,&k);
    char zimu[] = {"abcdefghijklmnopqrstuvwxyz"};
    int recordall[50] = {};
    for (int i = 0; i < strlen(str);i++)
    {
        int record = str[i] - 'a';
        record = (record + k) % 26;
        recordall[i] = record;
    }
    for (int i = 0; i < strlen(str);i++)
        printf("%c", zimu[recordall[i]]);
        return 0;
}