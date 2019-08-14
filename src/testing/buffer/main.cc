
#include "ByteBuffer.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	// ByteBuffer/NetBuffer
	ByteBuffer bb{10};

	bb.append("123456");
	bb.append("ab");
	bb.append("cd");

	ByteBuffer bb1 = bb;

	cout << "take before:" << bb << endl;
	cout << "take all:" << bb.taken_as_string() << endl;	// taken
	cout << "take after:" << bb << "|" << bb1 << endl;

	bb.append("01");
	bb.append("23");

	cout << "append success:" << bb.append("4567890") << endl;
	cout << "append success:" << bb.append("a") << endl;
	cout << "append rt:" << bb << endl;

	ByteBuffer bb2 = std::move(bb);
	std::cout << "std::move:" << bb2 << "|" << bb << std::endl;
}
