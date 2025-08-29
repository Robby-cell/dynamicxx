#include <dynamicxx/dynamicxx.h>
#include <gtest/gtest.h>

#include <cstddef>

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
    static constexpr auto FooBarValue = 42;

    Dynamic d = Dynamic::From<Dynamic::Object>();
    d[FooBarKey] = FooBarValue;

    EXPECT_TRUE(d.Contains(FooBarKey));
    EXPECT_EQ(d[FooBarKey], FooBarValue);
}

TEST(DynamicTest, CopyConstructionAndAssignment) {
    static constexpr std::size_t Size = 12;

    Dynamic d = Dynamic::From<Dynamic::Array>(Size);

    constexpr auto PerformAsserts = [](const Dynamic& d) {
        EXPECT_EQ(d.GetArray().size(), Size);
        for (const auto& value : d.GetArray()) {
            EXPECT_TRUE(value.IsUndefined());
        }
    };
    PerformAsserts(d);

    const auto copy = d;
    PerformAsserts(copy);  // Copy should be the same
    PerformAsserts(d);     // Original should also be like this

    d.Push(42);
    d.Push("Hello, world");
    d[1] = Dynamic::Array{};
    d[2] = 123;

    const auto another_copy = d;
    ASSERT_EQ(d, another_copy);  // They should be deeply equal
}
