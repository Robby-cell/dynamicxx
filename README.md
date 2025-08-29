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
    git clone <repository-url>
    cd dynamicxx
    ```

2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

3.  Configure and build:
    ```bash
    cmake ..
    cmake --build .
    ```

4.  Run examples and tests:
    ```bash
    ./example
    ./run_tests
    ```

## Usage

To use this library, simply add the `include` directory to your project's include paths.
```cpp
#include "dynamicxx/dynamicxx.hpp"
// ...
```
