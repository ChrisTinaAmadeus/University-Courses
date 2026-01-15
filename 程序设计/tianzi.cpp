#include <stdio.h>
#include <string.h>
#include<iostream>
using namespace std;
char board[2100][2100];
struct save
{
    int wei[10];
    int count;
};
save all[100];
int scount;
int kongbai(char b[][2100],int m,int n,int w,char word[])
{
    int judge=0;
    for(int i=0;i<m;i++)
    {
        for(int j=0;j<n;j++)
        {
            int ncountr1=0,ncountc1=0,ncountr2=0,ncountc2=0;
            int ncountR1=0,ncountC1=0,ncountR2=0,ncountC2=0;
            for(int k=0;k<w;k++)
            {
                if((b[i][j+k]=='0'||b[i][j+k]-'0'==word[ncountc1]-'0')&&j+k<n)
                {
                    ncountc1++;
                    if(b[i][j+k]-'0'==word[ncountc1-1]-'0')
                    ncountC1++;
                }
                if((b[i+k][j]=='0'||b[i+k][j]-'0'==word[ncountr1]-'0')&&i+k<m)
                {
                    ncountr1++;
                    if(b[i+k][j]-'0'==word[ncountr1-1]-'0')
                    ncountR1++;
                }
                if((b[i][j-k]=='0'||b[i][j-k]-'0'==word[ncountc2]-'0')&&j-k>=0)
                {
                    ncountc2++;
                    if(b[i][j-k]-'0'==word[ncountc2-1]-'0')
                    ncountC2++;
                }
                if((b[i-k][j]=='0'||b[i-k][j]-'0'==word[ncountr2]-'0')&&i-k>=0)
                {
                    ncountr2++;
                    if(b[i-k][j]-'0'==word[ncountr2-1]-'0')
                    ncountR2++;
                }
            }
            if(ncountr1==w&&(i==0||b[i-1][j]!='0')&&(i+w==m||b[i+w][j]!='0'))
            {
                judge=1;
                all[scount].wei[0]=i;
                all[scount].wei[1]=j;
                all[scount].wei[2]=i+w-1;
                all[scount].wei[3]=j;
                all[scount].count=ncountR1;
                scount++;
            }
            if(ncountc1==w&&(j==0||b[i][j-1]!='0')&&(j+w==n||b[i][j+w]!='0'))
            {
                judge=1;
                all[scount].wei[0]=i;
                all[scount].wei[1]=j;
                all[scount].wei[2]=i;
                all[scount].wei[3]=j-w+1;
                all[scount].count=ncountC1;
                scount++;
            }
            if(ncountr2==w&&(i==m-1||b[i+1][j]!='0')&&(i-w+1==0||b[i-w][j]!='0'))
            {
                judge=1;
                all[scount].wei[0]=i;
                all[scount].wei[1]=j;
                all[scount].wei[2]=i-w+1;
                all[scount].wei[3]=j;
                all[scount].count=ncountR2;
                scount++;
            }
            if(ncountc2==w&&(j==n-1||b[i][j+1]!='0')&&(j-w+1==0||b[i][j-w]!='0'))
            {
                judge=1;
                all[scount].wei[0]=i;
                all[scount].wei[1]=j;
                all[scount].wei[2]=i;
                all[scount].wei[3]=j-w+1;
                all[scount].count=ncountC2;
                scount++;
            }
        }
    }
    return judge;
}
int main() 
{
    int m,n;
    scanf("%d%d",&m,&n);
    getchar();
    for(int i=0;i<m;i++)
    {
        for(int j=0;j<n;j++)
        {
            scanf("%s",&board[i][j]);
        }
    }
    char word[2100]={};
    scanf("%s",word);
    int w=strlen(word);
    int judge1=kongbai(board,m,n,w,word);
    save temp;
    for(int i=0;i<scount;i++)
        for(int j=0;j<scount-1;j++)
        {
            if(all[j].count<all[j+1].count)
            {
                temp=all[j];
                all[j]=all[j+1];
                all[j+1]=temp;
            }
        }
    if(judge1)
    printf("%d %d\n%d %d",all[0].wei[0],all[0].wei[1],all[0].wei[2],all[0].wei[3]);
    else printf("No");
    return 0;
}