#include <gtest/gtest.h>
#include <textparser.hpp>

#include <cstring>

TEST(textparser_version, returns_configured_version)
{
    const char *version = textparser_version();

    ASSERT_NE(version, nullptr);
    EXPECT_GT(std::strlen(version), 0u);
    EXPECT_EQ(std::strcmp(version, TEXTPARSER_EXPECTED_VERSION), 0);
}

TEST(textparser_version, returns_stable_pointer)
{
    EXPECT_EQ(textparser_version(), textparser_version());
}

TEST(textparser_version, returns_numeric_components)
{
    EXPECT_EQ(textparser_version_major(), TEXTPARSER_EXPECTED_VERSION_MAJOR);
    EXPECT_EQ(textparser_version_minor(), TEXTPARSER_EXPECTED_VERSION_MINOR);
    EXPECT_EQ(textparser_version_patch(), TEXTPARSER_EXPECTED_VERSION_PATCH);
    EXPECT_EQ(textparser_version_int(), TEXTPARSER_EXPECTED_VERSION_INT);
}

TEST(textparser_version, packed_version_matches_components)
{
    const int expected = textparser_version_major() * 10000
                       + textparser_version_minor() * 100
                       + textparser_version_patch();
    EXPECT_EQ(textparser_version_int(), expected);
}
