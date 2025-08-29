#include <dynamicxx/dynamicxx.h>

#include <iostream>

using dynamicxx::Dynamic;

int main() {
    Dynamic d;

    d = 123;  // It will figure out which type to construct.

    d = 42.0F;  // It will figure out which type to create.
    std::cout << "Float value: " << d.GetNumber() << '\n';

    d = "Hello world";  // It will figure it out again.
    std::cout << "String value: " << d.GetString() << '\n';
}
