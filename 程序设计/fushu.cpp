#include <iostream>
using namespace std;
class Complex
{
	public:
	Complex(double r=0,double i=0)
	{
		shi = r;
		xu = i;
	}
	void show()
	{
		if(shi!=0&&xu!=0&&xu>0)
			cout << shi << '+' << xu << 'i' << endl;
		if(shi!=0&&xu!=0&&xu<0)
			cout << shi << xu << 'i' << endl;
		if(shi==0&&xu!=0)
			cout << xu << 'i' << endl;
		if(shi!=0&&xu==0)
			cout << shi << endl;
		if(shi==0&&xu==0)
			cout << '0' << endl;
	}
	void add(Complex &a)
	{
		shi += a.shi;
		xu += a.xu;
	}
	void sub(Complex &a)
	{
		shi -= a.shi;
		xu -= a.xu;
	}
	private:
		double shi;
		double xu;
};

int main()
{
	double re, im;
	cin >> re >> im;
	Complex c1(re, im);		// 用re, im初始化c1
	cin >> re;
	Complex c2 = re;     		// 用实数re初始化c2

	c1.show();
	c2.show();
	c1.add(c2);         // 将C1与c2相加，结果保存在c1中
	c1.show();          // 将c1输出
	c2.sub(c1);			// c2-c1，结果保存在c2中 
	c2.show();			// 输出c2 
	
    return 0;
}
