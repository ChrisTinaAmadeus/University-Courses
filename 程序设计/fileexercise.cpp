#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
struct PERSON
{
    int age;
    long long sno;
    char name[100];
    char identity[100];
};
void getpersons(FILE *f, int num, char s[])
{
    fgets(s, num, f);
    s[strlen(s) - 1] = '\0';
}
int main()
{
    PERSON person[100] = {};
    FILE *fp = fopen("example.txt", "r");
    int n;
    fscanf(fp, "%d", &n);
    fgetc(fp);
    for (int i = 1; i <= n; i++)
    {
        fgetc(fp);
        fscanf(fp, "%d", &person[i].age);
        fgetc(fp);
        fscanf(fp, "%lld", &person[i].sno);
        fgetc(fp);
        getpersons(fp, 100, person[i].name);
        getpersons(fp, 100, person[i].identity);
    }
    fclose(fp);
    FILE *fp1 = fopen("out.txt", "w");
    fprintf(fp1, "%d\n", n);
    for (int i = 1; i <= n; i++)
    {
        fprintf(fp1, "\n");
        fprintf(fp1, "%d\n", person[i].age);
        fprintf(fp1, "%lld\n", person[i].sno);
        fputs(person[i].name, fp1);
        fprintf(fp1, "\n");
        fputs(person[i].identity, fp1);
        fprintf(fp1, "\n");
    }
    fclose(fp1);
    return 0;
}