#include <dynamicxx/dynamicxx.h>
#include <gtest/gtest.h>

using dynamicxx::Dynamic;

TEST(DynamicTest, BasicDynamicText) {
    static constexpr auto StringToUse = "Foobar";
    Dynamic d = Dynamic::From<Dynamic::String>(StringToUse);

    std::string s = d;

    EXPECT_EQ(d.GetString(), StringToUse);
    EXPECT_EQ(d.GetString(), s);
}

TEST(DynamicTest, BasicTest) {
    static constexpr auto FooBarKey = "Foobar";

    Dynamic d = Dynamic::From<Dynamic::Object>();
    d[FooBarKey] = 42;

    EXPECT_TRUE(d.Contains(FooBarKey));
    EXPECT_EQ(d[FooBarKey], 42);
}
