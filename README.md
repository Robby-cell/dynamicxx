# dynamicxx

A header-only C++11 library.

## Building

This project uses CMake to build examples and tests.

### Prerequisites

*   CMake (version 3.10 or later)
*   A C++11 compatible compiler

### Instructions

1.  Clone the repository:
    ```bash
    git clone https://github.com/Robby-cell/dynamicxx.git
    cd dynamicxx
    ```

2.  Configure and build:
    ```bash
    cmake . --preset="debug"
    cmake --build . --preset="default"
    ```

3.  Run examples and tests:
    ```bash
    ./build/Debug/example
    ./build/Debug/run_tests
    ```

## Usage

To use this library, simply add the `include` directory to your project's include paths.
```cpp
#include "dynamicxx/dynamicxx.h"
#include <iostream>

int main() {
    using dynamicxx::Dynamic;

    Dynamic dynamic = Dynamic::From<Dynamic::String>("Hello, world!");
    std::cout << dynamic.GetString() << '\n';
}
```
