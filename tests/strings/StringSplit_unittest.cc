#include "strings/StringSplit.h"

#include <gtest/gtest.h>

using namespace annety;
using namespace std;

TEST (StringSplit_unittest, split_string)
{
	std::string input = "12, 345;67,890";
	std::vector<std::string> tokens 
			= split_string(input, ",;", KEEP_WHITESPACE, SPLIT_WANT_ALL);
	
	ASSERT_EQ(tokens.size(), 4);
	ASSERT_EQ(tokens[0], "12");
	ASSERT_EQ(tokens[1], " 345");
	ASSERT_EQ(tokens[2], "67");
	ASSERT_EQ(tokens[3], "890");
}

TEST (StringSplit_unittest, split_string_using_substr)
{
	std::string input = "12\r\n 345\r\n67\r\n890";
	std::vector<std::string> tokens 
			= split_string_using_substr(input, "\r\n", KEEP_WHITESPACE, SPLIT_WANT_ALL);
	
	ASSERT_EQ(tokens.size(), 4);
	ASSERT_EQ(tokens[0], "12");
	ASSERT_EQ(tokens[1], " 345");
	ASSERT_EQ(tokens[2], "67");
	ASSERT_EQ(tokens[3], "890");
}

TEST (StringSplit_unittest, split_string_into_key_value_pairs)
{
	StringPairs kvs;
	StringPiece str1("name=wlmwang;age=18");
	split_string_into_key_value_pairs(str1, '=', ';', &kvs);
	
	ASSERT_EQ(kvs.size(), 2);
	ASSERT_EQ(kvs[0].first, "name");
	ASSERT_EQ(kvs[0].second, "wlmwang");
	ASSERT_EQ(kvs[1].first, "age");
	ASSERT_EQ(kvs[1].second, "18");
}
