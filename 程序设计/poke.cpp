#include<stdio.h>
#include<math.h>
#include<string.h>
#include<iostream>
using namespace std;
struct card
{
    char se;
    char shu[3];
    int flag;
};
struct all
{
    card di[5];
    card publicpai[10];
    int num[20];
    char final[5];
    int finalnum[20];
};
int huase(card d[],card p[])
{
    int judge=0;
    if(d[0].se!=d[1].se)return judge;
    else
    {
        d[0].flag=1;
        d[1].flag=1;
            int ncount=0;
            for(int j=0;j<5;j++)
            {
                if(p[j].se==d[0].se)
                {
                    ncount++;
                    p[j].flag=1;
                }
            }
            if(ncount>=3)
            {
                judge=1;
            }
    return judge;
    }
}
void change(char a[],int num[],int judge);
int lianxu(all t)
{
    int judge=0;
    change(t.di[0].shu,t.num,1);
    change(t.di[1].shu,t.num,1);
    for(int i=0;i<5;i++)
    change(t.publicpai[i].shu,t.num,0);
    for(int i=1;i<=10&&(!judge);i++)
    {
        if(t.num[i]>0&&t.num[i+1]>0&&t.num[i+2]>0&&t.num[i+3]>0&&t.num[i+4]>0)
        {
            int jcount=0;
            for(int j=i;j<=i+4;j++)
            if(t.num[j]>=10)
            {
                jcount++;
            }
            if(jcount==2)
            {
                judge=1;
            }
        }
    }
    return judge;
}
void change(char a[],int num[],int judge)
{
    if(!judge)
    {
        if(a[0]=='J') num[11]++;
        if(a[0]=='Q') num[12]++;
        if(a[0]=='K') num[13]++;
        if(a[0]=='A') 
        {
            num[14]++;
            num[1]++;
        }
        if(a[0]!='J'&&a[0]!='Q'&&a[0]!='K'&&a[0]!='A'&&a[0]!='1') num[a[0]-'0']++;
        if(a[0]=='1') num[10]++;
    }
    else
    {
        if(a[0]=='J') num[11]+=10;
        if(a[0]=='Q') num[12]+=10;
        if(a[0]=='K') num[13]+=10;
        if(a[0]=='A') 
        {
            num[14]+=10;
            num[1]+=10;
        }
        if(a[0]!='J'&&a[0]!='Q'&&a[0]!='K'&&a[0]!='A'&&a[0]!='1') num[a[0]-'0']+=10;
        if(a[0]=='1') num[10]+=10;
    }
}
int finaljudge(all t)
{
    int judge=0;
    change(t.di[0].shu,t.finalnum,0);
    change(t.di[1].shu,t.finalnum,0);
    for(int i=0;i<5;i++)
    {
        if(t.publicpai[i].flag==1)
        {
            change(t.publicpai[i].shu,t.finalnum,0);
        }
    }
    for(int i=1;i<=10;i++)
    {
        if(t.finalnum[i]>0&&t.finalnum[i+1]>0&&t.finalnum[i+2]>0&&t.finalnum[i+3]>0&&t.finalnum[i+4]>0)
        {
            judge=1;
        }
    }
    return judge;
}
int main()
{
    int n;
    scanf("%d%*c",&n);
    all total[1200]={};
    for(int i=0;i<n;i++)
    {
        int judge1=1;
        for(int j=0;j<2;j++)
        {
            scanf("%c%s%*c",&total[i].di[j].se,&total[i].di[j].shu);
        }
        for(int j=0;j<5;j++)
        {
            scanf("%c%s%*c",&total[i].publicpai[j].se,&total[i].publicpai[j].shu);
        }
        if((huase(total[i].di,total[i].publicpai)&&!(lianxu(total[i])))||(huase(total[i].di,total[i].publicpai)&&lianxu(total[i])&&!(finaljudge(total[i]))))
        {
            total[i].final[0]='T';
            total[i].final[1]='H';
        }
        if(huase(total[i].di,total[i].publicpai)&&lianxu(total[i])&&finaljudge(total[i]))
        {
            total[i].final[0]='T';
            total[i].final[1]='H';
            total[i].final[2]='S';
        }
        if(!(huase(total[i].di,total[i].publicpai))&&lianxu(total[i]))
        {
            total[i].final[0]='S';
            total[i].final[1]='Z';
        }
        if(!(huase(total[i].di,total[i].publicpai))&&!(lianxu(total[i])))
        {
            total[i].final[0]='G';
            total[i].final[1]='P';
        }
    }
    for(int i=0;i<n;i++)
    printf("%s\n",total[i].final);
    return 0;
}