#include <stdio.h>

void word(char *s){
    char charactor[30]={};
    for(int i=0;;i++)
    {
        if(s[i]<'a'||s[i]>'z')
        break;
        int num = s[i] - 'a';
        charactor[num] += 1;
    }
    int manum = -1, minum = 200;
    for (int i = 0;i<26;i++)
    {
        if(charactor[i]==0)
            continue;
        manum = manum > charactor[i] ? manum : charactor[i];
        minum = minum < charactor[i] ? minum : charactor[i];
    }
    int finalnum = manum - minum;
    bool judge = true;
    if(finalnum<=1)
        judge = false;
    for (int i = finalnum - 1; i >= 2 && judge; i--)
    {
        if(finalnum%i==0)
            judge = false;
    }
    if(judge)
    {
        printf("Lucky Word\n%d",finalnum);
    }
    else
        printf("No Answer\n0");
}
int main(void){
    char s[102];
    scanf("%s",s);
    word(s);
    return 0;
}
