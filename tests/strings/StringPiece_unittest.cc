#include "strings/StringPiece.h"

#include <gtest/gtest.h>

using namespace annety;
using namespace std;

TEST (StringPiece_unittest, construct)
{
	std::string ss = "abcd";
	StringPiece sp(ss);

	ASSERT_EQ(sp.size(), ss.size());
	ASSERT_TRUE(::memcmp(sp.as_string().c_str(), ss.c_str(), ss.size()) == 0);

	ASSERT_EQ(sp.front(), 'a');
	ASSERT_EQ(sp[3], 'd');

	sp.clear();
	ASSERT_TRUE(sp.empty());
}

TEST (StringPiece_unittest, compare)
{
	StringPiece sp("abcd");

	ASSERT_TRUE(sp == "abcd");
	ASSERT_TRUE(sp != "abcde");
	ASSERT_TRUE(sp > "abcc");
	ASSERT_TRUE(sp < "abcde");

	ASSERT_TRUE(sp.starts_with("abc"));
	ASSERT_TRUE(sp.ends_with("bcd"));
}

TEST (StringPiece_unittest, copy)
{
	StringPiece sp("abcd");
	char cs[5]{'\0'};

	sp.copy(cs, 2);
	ASSERT_TRUE(::memcmp(cs, "ab", 2) == 0);

	sp.copy(cs, 2, 2);
	ASSERT_TRUE(::memcmp(cs, "cd", 2) == 0);
}

TEST (StringPiece_unittest, substr)
{
	StringPiece sp("abcdefabcd");

	ASSERT_TRUE(sp.substr(0, 4) == "abcd");
	ASSERT_TRUE(sp.substr(1, 4) == "bcde");
	ASSERT_TRUE(sp.substr(6) == "abcd");
}

TEST (StringPiece_unittest, find)
{
	StringPiece sp("abcd\r\nefabcd\r");

	ASSERT_EQ(sp.find('\r'), 4);
	ASSERT_EQ(sp.find("\r\n"), 4);

	ASSERT_EQ(sp.rfind('\r'), 12);
	ASSERT_EQ(sp.rfind("\r\n"), 4);

	ASSERT_TRUE(sp.rfind("\t") == StringPiece::npos);
}
