#include <iostream>
#include <math.h>
#include<iomanip>

using namespace std;

class Rectangle{

public:
//在下面的空格声明 Rectangle 类的成员函数
    Rectangle(double X1, double Y1, double X2, double Y2)
    {
        _x1 = X1;
        _x2 = X2;
        _y1 = Y1;
        _y2 = Y2;
    }
    Rectangle(Rectangle &other)
    {
        _x1=other._x1  ;
        _y1=other._y1  ;
       _x2= other._x2  ;
        _y2=other._y2  ;
    }
    void compute();
    void print();

private:
    //左下角坐标
    double _x1;
    double _y1;

    //右上角坐标
    double _x2;
    double _y2;

    //宽度和高度
    double _width;
    double _height;
};

//在下面的空格实现 Rectangle 类的成员函数
void Rectangle::compute()
{
    _width = fabs(_x2 - _x1);
    _height =fabs(_y2 - _y1);
}
void Rectangle::print()
{
    cout << fixed << setprecision(2) << (_width + _height) * 2 << endl;
    cout << fixed << setprecision(2) << ( _width * _height) << endl;
    cout << fixed << setprecision(2) << (_width + _height) * 4 << endl;
    cout << fixed << setprecision(2) << (_width * _height) * 2 << endl;
}
int main(){

    double x1, x2, y1, y2;
    cin >>x1>>y1>>x2>>y2;
    Rectangle r1(x1,y1,x2,y2);

    Rectangle r2 = r1;

//在下面的空格按题目要求输出结果
    r1.compute();
    r1.print();

    return 0;
}
