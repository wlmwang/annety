
#include "containers/Bind.h"

#include <iostream>

using namespace annety;
using namespace std;

void TestFun(int a, int b, int c)
{
	cout << "~~TestFun~~" << a << b << c << endl;
}

class WapperCall
{
public:
	void test(int c) {
		cout << "WapperCall::test:" << c << "|" << v_ << endl;
	}
private:
	int v_{1};
};

int main(int argc, char* argv[])
{
	// WBindFuctor
	std::function<void(int)> xx;
	{
		xx = containers::make_bind(TestFun, 1, 2, containers::_1);
		xx(3);
		
		std::shared_ptr<WapperCall> call(new WapperCall());
		xx = containers::make_bind(&WapperCall::test, call, containers::_1);
		cout << "use cout:" << call.use_count() << endl;
		xx(10);

		xx = containers::make_weak_bind(&WapperCall::test, call, containers::_1);
		cout << "use cout:" << call.use_count() << endl;
		xx(11);

		std::weak_ptr<WapperCall> wcall = call;
		xx = containers::make_weak_bind(&WapperCall::test, wcall, containers::_1);
		cout << "use cout:" << call.use_count() << endl;
		xx(12);
	}
	if (xx) xx(20);
}
