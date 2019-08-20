
#include "strings/StringPiece.h"

#include <string>
#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	StringPiece str("abcd\r\nefabcd\r\n");

	StringPiece eof("\r\n");
	StringPiece::size_type n = str.find(eof, 0);

	std::cout << n << std::endl;

	StringPiece one(str.data(), n);

	std::cout << one << std::endl;
}
