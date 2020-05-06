#include "strings/StringPrintf.h"

#include <gtest/gtest.h>

using namespace annety;
using namespace std;

TEST (StringPrintf_unittest, string_printf)
{
	string sf = string_printf("%3.1f", 123.456);
	ASSERT_EQ(sf, "123.5");
	
	string sf2 = string_printf("%1.2f", 123.456);
	ASSERT_EQ(sf2, "123.46");
	
	string sf3 = string_printf("%5.3f", 123.456);
	ASSERT_EQ(sf3, "123.456");
}

TEST (StringPrintf_unittest, sstring_printf)
{
	string sf;
	sstring_printf(&sf, "%3.1f", 123.456);
	ASSERT_EQ(sf, "123.5");
	
	string sf2;
	sstring_printf(&sf2, "%1.2f", 123.456);
	ASSERT_EQ(sf2, "123.46");
	
	string sf3;
	sstring_printf(&sf3, "%5.3f", 123.456);
	ASSERT_EQ(sf3, "123.456");
}

TEST (StringPrintf_unittest, sstring_appendf)
{
	string sf = "123.4";
	
	sstring_appendf(&sf, "*%s", "456.7");
	ASSERT_EQ(sf, "123.4*456.7");
}
