#include<stdio.h>
#include<iostream>
#include<cmath>
using namespace std;
int main()
{
    int a,b,c,d,e,f,g,h,i,j,k,l,m;
    char x[15];//char中的数字都是字符，以ASCII表中的数储存，'0'=48
    fgets(x,sizeof(x),stdin);//读入一串字符记入x[]中
    sscanf(x,"%d-%d-%d",&a,&b,&c);//取三个整数赋值到a,b,c中
    e=b/100;                     //%1d的话就可以直接读到十个变量里
    f=(b%100)/10;
    g=b%10;
    h=c/10000;
    i=(c%10000)/1000;
    j=(c%1000)/100;
    k=(c%100)/10;
    l=c%10;
    m=a*1+e*2+f*3+g*4+h*5+i*6+j*7+k*8+l*9;
    if(x[12]=='X')//判断d的值
        d=10;
    else
    d=x[12]-'0';//-'0'是为了使d为输入的那个值而非ASCII里的数
    if(m%11==d)
    printf("Right");
    else
    {
        if(m%11==10)
        printf("%d-%d-%d-X",a,b,c);
        else
        printf("%d-%d-%d-%d",a,b,c,m%11);
    }
    return 0;;
}