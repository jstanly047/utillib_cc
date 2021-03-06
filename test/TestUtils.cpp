#include <Utils.h>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>



class TestUtils : public ::testing::Test
{
};


TEST_F(TestUtils, stringToEpoc)
{
    ASSERT_EQ(bpms::Utils::getDateTime("19700101073000"), 0);
    ASSERT_EQ(bpms::Utils::getDateTime("1970/01/01 07:30:00", "%Y/%m/%d %H:%M:%S"), 0);
}

TEST_F(TestUtils, epocToStr)
{
    ASSERT_EQ(bpms::Utils::getDateTimeStr(0), "19700101073000");
    ASSERT_EQ(bpms::Utils::getDateTimeStr(0,"%Y/%m/%d %H:%M:%S"), "1970/01/01 07:30:00");
}

TEST_F(TestUtils, charSplit)
{
    auto retVal = bpms::Utils::splitString<char, char>("A,B,C", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre('A','B','C'));
}

TEST_F(TestUtils, intSplit)
{
    auto retVal = bpms::Utils::splitString<int, char>("1,2,3", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre(1,2,3));
}

TEST_F(TestUtils, stringSplit)
{
    auto retVal = bpms::Utils::splitString<std::string, char>("AB,CD,EF", ',');
    ASSERT_THAT(retVal, ::testing::ElementsAre("AB", "CD", "EF"));
}

TEST_F(TestUtils, stringSplitBySplit)
{
    auto retVal = bpms::Utils::splitString<std::string, std::string>("AB==CD==EF", "==");
    ASSERT_THAT(retVal, ::testing::ElementsAre("AB", "CD", "EF"));
}

TEST_F(TestUtils, testTimeSizeIs64Bit)
{
    ASSERT_EQ(sizeof(std::time_t), 8U);
}