#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;
char all[200][500];
int main()
{
    int n;
    scanf("%d", &n);
    getchar();
    for (int i = 0; i < n; i++)
    {
        gets(all[i]);
    }
    char record[250]={};
    for(int i=0;i<n;i++)
    {
        int j=0;
        int leni=strlen(all[i]);
        int r=0;
        bool judge1=true;
        while (j<leni&&judge1)
        {
            if(all[i][j]=='(')
            record[r++]=1;
            if(all[i][j]=='{')
            record[r++]=2;
            if(all[i][j]==')')
            {
                if(record[r-1]!=1)
                judge1=false;
                else 
                {
                    record[r-1]=0;
                    r-=1;
                }
            }
            if(all[i][j]=='}')
            {
                if(record[r-1]!=2)
                judge1=false;
                else 
                {
                    record[r-1]=0;
                    r-=1;
                }
            }
            j++;
        }
        if(strlen(record)!=0)
        judge1=false;
        if(judge1)printf("true\n");
        else printf("false\n");
    }
        return 0;
}