#include<iostream>
using namespace std;
int main()
{
    char a,b;
    cin>>a>>b;
    int c=static_cast<int>(a);
    int d=static_cast<int>(b);
    cout<<a<<","<<b<<endl;
    cout<<c<<","<<d;
    return 0;
}