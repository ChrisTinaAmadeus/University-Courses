#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;
#define LINE_BUF_SIZE 2048

typedef struct

{

    char isbn[11]; // 10 digits isbn

    char name[LINE_BUF_SIZE];

    char author[LINE_BUF_SIZE];

    char publisher[LINE_BUF_SIZE];

    int year;

} book_record_t;
typedef struct
{

    // 指向一个存放所有图书数据的数组。

    // 特别注意：函数调用时，本数组并未分配空间，需要你根据读入的数据合理地动态分配空间

    book_record_t *data;

    int size; // 图书库中记录的条数（即数组元素的个数)

} book_manager_t;
int book_manager_load(book_manager_t *mgr, char *file_name)
{
    FILE *fp;
    fp = fopen(file_name, "r");
    if (fp == NULL)
        return -1;
    int n;
    fscanf(fp, "%d", &n);
    mgr->data = (book_record_t *)malloc(n * sizeof(book_record_t));
    mgr->size = n;
    fgetc(fp); // 跳过数字后的回车
    for (int i = 0; i < n; i++)
    {
        fgetc(fp); // 跳过空行
        book_record_t *a = &(mgr->data[i]);
        fgets(a->isbn, 11, fp);
        fgetc(fp); // isbn号最后一个换行符要跳过
        fgets(a->name, 2048, fp);
        a->name[strlen(a->name) - 1] = '\0'; //'\n'->'\0'
        fgets(a->author, 2048, fp);
        a->author[strlen(a->author) - 1] = '\0';
        fgets(a->publisher, 2048, fp);
        a->publisher[strlen(a->publisher) - 1] = '\0';
        fscanf(fp, "%d", &a->year);
        fgetc(fp); // 跳过数字后的回车
    }
    fclose(fp);
    return 0;
}
int main()
{
    char *file_name = "example.txt";
    book_manager_t *all = NULL;
    book_manager_load(all, file_name);
    return 0;
}