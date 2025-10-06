#include <iostream>

#include "include/obfuscxx.h"

int main()
{
    obfuscxx<int> int_value{ 100 };
    std::cout << int_value.get() << '\n'; // 100
    int_value = 50;
    std::cout << int_value.get() << '\n'; // 50

    obfuscxx<float> float_value{ 1.5f };
    std::cout << float_value.get() << '\n'; // 1.5f

    obfuscxx<int, 4> array{ 1, 2, 3, 4 };
    for (auto val : array) {
        std::cout << val << " "; // 1 2 3 4
    }
    std::cout << '\n';

	obfuscxx str("str");
    std::cout << str.to_string() << '\n'; // str

    obfuscxx<int*> pointer{};
    pointer = new int{101};
    std::cout << pointer.get() << " " << *pointer.get() << '\n'; // ptr, 101
    delete pointer.get();
}