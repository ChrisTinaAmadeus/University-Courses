#include<stdio.h>
#include<string.h>

char parseCmd(char *s, char *str1, char *str2)
{
    for(int i=0;i<500;i++)
    {
        str1[i] = 0;
        str2[i] = 0;
    }
    if(s[0]=='C'||s[0]=='D')
    {
        for (int i = 2;;i++)
        {
            if(s[i]=='\0')
                break;
            else
                str1[i - 2] = s[i];
        }
    }
    else 
    {
        int ncount = 0;
        for (int i = 2;; i++)
        {
            if (s[i] == ' ')
                break;
            else
            {
                str1[i - 2] = s[i];
                ncount++;
            }
        }
        for (int i = ncount+3;; i++)
        {
            if (s[i] == '\0')
                break;
            else
            {
                str2[i - ncount-3] = s[i];
            }
        }
    }
    return s[0];
}
void setzero(char *w,int n)
{
    for(int i=0;i<n;i++)
        *(w+i) = 0;
}
int countStr(char *s,char *str1)
{
    int len = strlen(s);
    int l = strlen(str1);
    int sum = 0;
    for (int i = 0;i<len;)
    {
        if(strncmp(s+i,str1,l)==0)
        {
            sum++;
            i += l;
        }
        else
            i++;
    }
    return sum;
}
void deleteStr(char *s,char *str1)
{
    int len = strlen(s);
    setzero(s + len,500-len);
    int l = strlen(str1);
    char *wei = strstr(s, str1);
    bool judge = (wei == NULL);
    if(judge)
        return;
    char last[500]={},change[500]={};
    strcpy(last, wei+l);
    strncpy(change, s, wei - s);
    setzero(s, 500);
    strcpy(s, change);
    strcat(s, last);
}
void insertStr(char *s,char *str1,char *str2)
{
    int len = strlen(s);
    int l = strlen(str1);
    char *wei=NULL;
    for (int i = 0; i < len; i++)
    {
        if (strncmp(s + i, str1, l) == 0)
            wei = &s[i];
    }
    bool judge=(wei==NULL);
    if(judge)
        return;
    char last[500] = {}, first[500] = {};
    strncpy(first, s, wei - s);
    strcpy(last, wei);
    setzero(s,500);
    strcpy(s,first);
    strcat(s,str2);
    strcat(s, last);
}
char *replaceStr(char *s, char *str1, char *str2)
{
    int len = strlen(s);
    int l = strlen(str1),l2=strlen(str2);
    char *wei;
    char last[500] = {}, first[500] = {};
    for (int i = 0; i<len;)
    {
        if (strncmp(s + i, str1, l) == 0)
        {
            wei = &s[i];
            strncpy(first, s, wei - s);
            strcpy(last, wei + l);
            setzero(s, 500);
            strcpy(s,first);
            strcat(s,str2);
            strcat(s, last);
            i += l2;
            len = strlen(s);
        }
        else
            i++;
    }
    return s;
}

int main()
{
	char s[500], cmdStr[500], str1[500], str2[500], cmd;
	char *pStr1, *pStr2;

    gets(s);
	gets(cmdStr);
    pStr1 = str1;
    pStr2 = str2;

    cmd = parseCmd(cmdStr, pStr1, pStr2);

    switch ( cmd )
    {
    case 'C':
        printf("%d\n", countStr(s, pStr1));
        break;
    case 'D':
        deleteStr(s, pStr1);
        printf("%s\n", s);
        break;
    case 'I':
        insertStr(s, pStr1, pStr2);
        printf("%s\n", s);
        break;
    case 'R':
        printf("%s\n", replaceStr(s, pStr1, pStr2));
        break;
    }
    return 0;
}
