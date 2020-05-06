#include "files/FilePath.h"

#include <gtest/gtest.h>

using namespace annety;
using namespace std;

TEST (FilePath_unittest, construct)
{
	FilePath fp("/usr/local/annety.log.gz");

	ASSERT_EQ(fp.dirname().value(), "/usr/local");
	ASSERT_EQ(fp.basename().value(), "annety.log.gz");

	ASSERT_TRUE(fp.is_absolute());
	ASSERT_FALSE(fp.references_parent());
}

TEST (FilePath_unittest, extension)
{
	FilePath fp("/usr/local/annety.log.gz");

	ASSERT_TRUE(fp.matches_extension(".log.gz"));
	ASSERT_EQ(fp.extension(), ".log.gz");
	ASSERT_EQ(fp.final_extension(), ".gz");
	
	ASSERT_EQ(fp.remove_extension().value(), "/usr/local/annety");
	ASSERT_EQ(fp.replace_extension("tgz").value(), "/usr/local/annety.tgz");
}

TEST (FilePath_unittest, get_components)
{
	FilePath fp("/usr/local/annety.log.gz");

	std::vector<std::string> components;
	fp.get_components(&components);

	ASSERT_EQ(components.size(), 4);
	ASSERT_EQ(components[0], "/");
	ASSERT_EQ(components[1], "usr");
	ASSERT_EQ(components[2], "local");
	ASSERT_EQ(components[3], "annety.log.gz");
}

