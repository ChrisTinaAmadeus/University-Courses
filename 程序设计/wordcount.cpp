#include<stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#include<string.h>
#include<iostream>
using namespace std;
char sentence[220][1050];
struct jilu
{
    int onetime=1;
    int times;
    char word[30];
};
jilu words[2000];
int main()
{
    int D,K;
    scanf("%d%d",&D,&K);
    getchar();
    int ncount=0;
    for(int i=0;i<D;i++)
    {
        gets(sentence[i]);
        int length=0;
        for(int j=0;j<strlen(sentence[i]);j++)
        {
            if(sentence[i][j]>=65&&sentence[i][j]<97)
            sentence[i][j]+=('a'-'A');
            if(sentence[i][j]!=' ')
            words[ncount].word[length++]=sentence[i][j];
            if(sentence[i][j]==' '||j==strlen(sentence[i])-1)
            {
                if(ncount>0)
                {
                    int judgew=1;
                    for(int k=ncount-1;k>=0&&judgew;k--)
                    {
                        if(strcmp(words[ncount].word,words[k].word)==0)
                        {
                            judgew=0;
                            if(words[k].onetime==1)
                            {
                                words[k].times++;
                                words[k].onetime=0;
                            }
                        }
                    }
                    if(!judgew)
                    {
                        for(int k=0;k<length;k++)
                        words[ncount].word[k]=0;
                        length=0;
                    }
                    else 
                    {
                        words[ncount].times++;
                        words[ncount].onetime=0;
                        ncount++;
                        length=0;
                    }
                }
                else 
                {
                    words[ncount].times++;
                    words[ncount].onetime=0;
                    ncount++;
                    length=0;
                }
            }
            
        }
        for(int k=0;k<=ncount;k++)
        words[k].onetime=1;
    }
    jilu temp;
    for(int i=0;i<ncount;i++)
        for(int j=0;j<ncount-1;j++)
        {
            if(words[j].times<words[j+1].times)
            {
                temp=words[j+1];
                words[j+1]=words[j];
                words[j]=temp;
            }
            else if(words[j].times==words[j+1].times&&strcmp(words[j].word,words[j+1].word)>0)
            {
                temp=words[j+1];
                words[j+1]=words[j];
                words[j]=temp;
            }
        }
    for(int i=0;i<K;i++)
    printf("%s %d\n",words[i].word,words[i].times);
    return 0;
}