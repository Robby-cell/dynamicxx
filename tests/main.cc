#include "dynamicxx/dynamicxx.h"
#include "gtest/gtest.h"

using dynamicxx::Dynamic;

TEST(DynamicTest, BasicDynamicText) {
    static constexpr auto StringToUse = "Foobar";
    Dynamic d = Dynamic::From<Dynamic::String>(StringToUse);

    EXPECT_EQ(d.GetString(), StringToUse);
}

TEST(DynamicTest, BasicTest) {}
