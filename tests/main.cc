#include <dynamicxx/dynamicxx.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <utility>

using dynamicxx::Dynamic;
using dynamicxx::DynamicManaged;

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
        EXPECT_EQ(d.size(), Size);
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

TEST(DynamicTest, BasicDynamic_Of) {
    static constexpr auto FortyTwo = 42;
    const auto integer = Dynamic::Of(FortyTwo);
    EXPECT_EQ(integer, FortyTwo);

    static constexpr auto TheString = "Foobar string.";
    const auto string = Dynamic::Of(TheString);
    EXPECT_EQ(string, TheString);

    auto number = Dynamic::Of(32.0);
    EXPECT_TRUE(number.IsNumber());
    auto stolen_number = std::move(number);
    EXPECT_TRUE(stolen_number.IsNumber());
    EXPECT_TRUE(number.IsUndefined());
    EXPECT_FALSE(number.IsNumber());
}

TEST(DynamicTest, ManagedVersion) {
    static constexpr std::array Values{1, 2, 42};

    DynamicManaged managed;
    managed.Emplace<DynamicManaged::Array>();

    for (const auto& v : Values) {
        managed.Push(v);
    }

    ASSERT_EQ(managed.size(), Values.size());
    {
        for (std::size_t i = 0, size = Values.size(); i < size; ++i) {
            ASSERT_EQ(Values[i], managed[i]);
        }
    }
}

TEST(DynamicTest, ManagedDeepClone) {
    static constexpr auto FooBarKey = "FooBar";

    DynamicManaged d;
    d.Emplace<DynamicManaged::Object>();

    d[FooBarKey] = 42;

    auto clone = d.Clone();

    ASSERT_EQ(d[FooBarKey], clone[FooBarKey]);
}
