#pragma once
#include <gtest/gtest.h>

#include "include/obfuscxx.h"

TEST(ObfuscxxTest, IntegerValue)
{
    obfuscxx<int> int_value{ 100 };
    EXPECT_EQ(int_value.get(), 100);

    int_value = 50;
    EXPECT_EQ(int_value.get(), 50);
}

TEST(ObfuscxxTest, FloatValue) {
    obfuscxx<float> float_value{ 1.5f };
    EXPECT_FLOAT_EQ(float_value.get(), 1.5f);
}

TEST(ObfuscxxTest, ArrayIteration) {
    obfuscxx<int, 4> array{ 1, 2, 3, 4 };
    int expected[] = { 1, 2, 3, 4 };
    int i = 0;
    for (auto val : array) {
        EXPECT_EQ(val, expected[i++]);
    }
}

TEST(ObfuscxxTest, StringValues) {
    obfuscxx str("str");
    EXPECT_STREQ(str.to_string(), "str");

    obfuscxx wstr(L"wstr");
    EXPECT_STREQ(wstr.to_string(), L"wstr");
}

TEST(ObfuscxxTest, PointerValue) {
    obfuscxx<int*> pointer{};
    pointer = new int{ 101 };
    EXPECT_NE(pointer.get(), nullptr);
    EXPECT_EQ(*pointer.get(), 101);
    delete pointer.get();
}

TEST(ObfuscxxTest, ComparisonOperators) {
    obfuscxx<int> a{ 100 };
    obfuscxx<int> b{ 100 };
    obfuscxx<int> c{ 50 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(c < a);
    EXPECT_TRUE(a > c);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(c <= a);
}

TEST(ObfuscxxTest, ArithmeticOperators) {
    obfuscxx<int> a{ 10 };
    obfuscxx<int> b{ 5 };

    EXPECT_EQ(a + b, 15);
    EXPECT_EQ(a - b, 5);
    EXPECT_EQ(a * b, 50);
    EXPECT_EQ(a / b, 2);

    a += b;
    EXPECT_EQ(a.get(), 15);

    a -= b;
    EXPECT_EQ(a.get(), 10);
}

TEST(ObfuscxxTest, ObfuscationLevels) {
    obfuscxx<int, 1, obf_level::Low> low{ 42 };
    obfuscxx<int, 1, obf_level::Medium> medium{ 42 };
    obfuscxx<int, 1, obf_level::High> high{ 42 };

    EXPECT_EQ(low.get(), 42);
    EXPECT_EQ(medium.get(), 42);
    EXPECT_EQ(high.get(), 42);
}

TEST(ObfuscxxTest, EdgeCases) {
    obfuscxx<int> max_int{ INT_MAX };
    obfuscxx<int> min_int{ INT_MIN };
    obfuscxx<int> zero{ 0 };
    obfuscxx<int> negative{ -12345 };

    EXPECT_EQ(max_int.get(), INT_MAX);
    EXPECT_EQ(min_int.get(), INT_MIN);
    EXPECT_EQ(zero.get(), 0);
    EXPECT_EQ(negative.get(), -12345);
}

TEST(ObfuscxxTest, FloatEdgeCases) {
    obfuscxx<float> zero{ 0.0f };
    obfuscxx<float> negative{ -3.14f };
    obfuscxx<float> small{ 0.0001f };
    obfuscxx<float> large{ 123456.789f };

    EXPECT_FLOAT_EQ(zero.get(), 0.0f);
    EXPECT_FLOAT_EQ(negative.get(), -3.14f);
    EXPECT_FLOAT_EQ(small.get(), 0.0001f);
    EXPECT_FLOAT_EQ(large.get(), 123456.789f);
}

TEST(ObfuscxxTest, ArrayOperators) {
    obfuscxx<int, 5> array{ 10, 20, 30, 40, 50 };

    EXPECT_EQ(array[0], 10);
    EXPECT_EQ(array[2], 30);
    EXPECT_EQ(array[4], 50);

    EXPECT_EQ(array.get(1), 20);
    EXPECT_EQ(array.get(3), 40);

    EXPECT_EQ(array.size(), 5);
}

TEST(ObfuscxxTest, ArraySet) {
    obfuscxx<int, 3> array{ 1, 2, 3 };

    array.set(100, 0);
    array.set(200, 1);
    array.set(300, 2);

    EXPECT_EQ(array[0], 100);
    EXPECT_EQ(array[1], 200);
    EXPECT_EQ(array[2], 300);
}

TEST(ObfuscxxTest, ArrayCopyTo) {
    obfuscxx<int, 5> array{ 1, 2, 3, 4, 5 };
    int output[5] = { 0 };

    array.copy_to(output, 5);

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(output[i], i + 1);
    }
}

TEST(ObfuscxxTest, ArrayAssignment) {
    obfuscxx<int, 3> array{ 1, 2, 3 };

    array = { 10, 20, 30 };

    EXPECT_EQ(array[0], 10);
    EXPECT_EQ(array[1], 20);
    EXPECT_EQ(array[2], 30);
}

TEST(ObfuscxxTest, DataIsEncrypted) {
    obfuscxx<int> value{ 42 };

    const uint64_t* raw_data = reinterpret_cast<const uint64_t*>(&value);
    volatile uint64_t encrypted = *raw_data;

    EXPECT_NE(encrypted, 42);

    EXPECT_EQ(value.get(), 42);
}

TEST(ObfuscxxTest, PointerOperators) {
    obfuscxx<int*> ptr{};
    ptr = new int{ 999 };

    EXPECT_NE(ptr.get(), nullptr);

    EXPECT_EQ(*ptr.get(), 999);

    *ptr.get() = 111;
    EXPECT_EQ(*ptr.get(), 111);

    delete ptr.get();
}

TEST(ObfuscxxTest, LongString) {
    obfuscxx<char, 48, obf_level::Low> str("this is a very long test string for obfuscation");
    auto result = str.to_string();
    EXPECT_STREQ(result, "this is a very long test string for obfuscation");
}

TEST(ObfuscxxTest, ConstCorrectness) {
    const obfuscxx<int> const_value{ 42 };

    EXPECT_EQ(const_value.get(), 42);

    EXPECT_EQ(const_value(), 42);
}

TEST(ObfuscxxTest, IteratorOperations) {
    obfuscxx<int, 5> array{ 1, 2, 3, 4, 5 };

    auto it = array.begin();
    EXPECT_EQ(*it, 1);

    ++it;
    EXPECT_EQ(*it, 2);

    EXPECT_NE(it, array.end());

    int count = 0;
    for (auto it = array.begin(); it != array.end(); ++it) {
        count++;
    }
    EXPECT_EQ(count, 5);
}

TEST(ObfuscxxTest, DifferentTypes) {
    obfuscxx<uint64_t> u64{ 0xFFFFFFFFFFFFFFFF };
    obfuscxx<int8_t> i8{ -127 };
    obfuscxx<double> dbl{ 3.141592653589793 };

    EXPECT_EQ(u64.get(), 0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ(i8.get(), -127);
    EXPECT_DOUBLE_EQ(dbl.get(), 3.141592653589793);
}

TEST(ObfuscxxTest, MultipleAssignments) {
    obfuscxx<int> value{ 10 };

    value = 20;
    EXPECT_EQ(value.get(), 20);

    value = 30;
    EXPECT_EQ(value.get(), 30);

    value = 40;
    EXPECT_EQ(value.get(), 40);
}

TEST(ObfuscxxTest, ArrayEquality) {
    obfuscxx<int, 3> a{ 1, 2, 3 };
    obfuscxx<int, 3> b{ 1, 2, 3 };
    obfuscxx<int, 3> c{ 1, 2, 4 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(ObfuscxxTest, ImplicitConversion) {
    obfuscxx<int> value{ 42 };

    int x = value;
    EXPECT_EQ(x, 42);

    int result = value + 10;
    EXPECT_EQ(result, 52);
}

TEST(ObfuscxxTest, RValueDefines) {
    EXPECT_STREQ(obfuss("str"), "str");
    EXPECT_STREQ(obfuss(L"wstr"), L"wstr");
    EXPECT_EQ(obfusv(52), 52);
    EXPECT_EQ(obfusv(3.14f), 3.14f);
    EXPECT_EQ(obfusv(-3.14f), -3.14f);
}

#if defined(__clang__) || defined(__GNUC__)
TEST(ObfuscxxTest, UserDefinedLiterals) {
    EXPECT_STREQ("str"_obf, "str");
    EXPECT_STREQ(L"wstr"_obf, L"wstr");
}
#endif