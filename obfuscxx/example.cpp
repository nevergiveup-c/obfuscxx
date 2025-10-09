#include <iostream>

#include "include/obfuscxx.h"

int main()
{
    obfuscxx<int> int_value{ 100 };
    std::cout << int_value.get() << '\n';
    int_value = 50;
    std::cout << int_value.get() << '\n';

    obfuscxx<float> float_value{ 1.5f };
    std::cout << float_value.get() << '\n';

    obfuscxx<int, 4> array{ 1, 2, 3, 4 };
    for (auto val : array) {
        std::cout << val << " ";
    }
    std::cout << '\n';

	obfuscxx str("str");
    std::cout << str.to_string() << '\n';

    obfuscxx<int*> pointer{};
    pointer = new int{101};
    std::cout << pointer.get() << " " << *pointer.get() << '\n';
    delete pointer.get();

    obfuscxx<std::string*> pointer1{};
    pointer1 = new std::string{"str2"};
    std::cout << pointer1->data() << '\n';

    delete pointer1.get();
}