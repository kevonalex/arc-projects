# C++ Comprehensive Cheat Sheet

## Table of Contents
1. [Basic Program Structure](#basic-program-structure)
2. [Data Types](#data-types)
3. [Header Files](#header-files)
4. [Multiple Files](#multiple-files)
5. [Compilation & Running](#compilation--running)
6. [Functions](#functions)
7. [Classes](#classes)
8. [Structs](#structs)
9. [Pointers & References](#pointers--references)
10. [Common Patterns](#common-patterns)

**📚 Related Guides:**
- [C++ Memory Management Guide](./C++_MEMORY_MANAGEMENT.md) - Comprehensive memory management
- [C++ Quant Developer Guide](./C++_QUANT_DEVELOPER_GUIDE.md) - Advanced topics for quantitative developers

---

## Basic Program Structure

### Minimal C++ Program
```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

### With Command Line Arguments
```cpp
#include <iostream>

int main(int argc, char* argv[]) {
    // argc: number of arguments
    // argv: array of argument strings
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
    }
    return 0;
}
```

---

## Data Types

### Fundamental Types

#### Integer Types
```cpp
// Signed integers
int x = 42;                    // At least 16 bits, typically 32
short s = 10;                  // At least 16 bits
long l = 1000L;                // At least 32 bits
long long ll = 1000000LL;      // At least 64 bits

// Unsigned integers
unsigned int ui = 42U;
unsigned short us = 10;
unsigned long ul = 1000UL;
unsigned long long ull = 1000000ULL;

// Fixed-width (C++11, requires <cstdint>)
#include <cstdint>
int8_t i8 = 127;               // Exactly 8 bits, signed
uint8_t u8 = 255;              // Exactly 8 bits, unsigned
int16_t i16 = 32767;
uint16_t u16 = 65535;
int32_t i32 = 2147483647;
uint32_t u32 = 4294967295U;
int64_t i64 = 9223372036854775807LL;
uint64_t u64 = 18446744073709551615ULL;
```

#### Floating Point Types
```cpp
float f = 3.14f;               // Single precision (32 bits)
double d = 3.14159265359;      // Double precision (64 bits)
long double ld = 3.141592653589793238L;  // Extended precision
```

#### Character Types
```cpp
char c = 'A';                  // At least 8 bits
signed char sc = -128;
unsigned char uc = 255;
wchar_t wc = L'Ω';             // Wide character
char16_t c16 = u'Ω';          // UTF-16 (C++11)
char32_t c32 = U'Ω';          // UTF-32 (C++11)
```

#### Boolean Type
```cpp
bool b1 = true;
bool b2 = false;
bool b3 = 1;                   // Non-zero is true
bool b4 = 0;                   // Zero is false
```

#### Void Type
```cpp
void function();               // Function returns nothing
void* ptr;                     // Pointer to unknown type
```

### Type Modifiers
```cpp
const int ci = 42;             // Constant, cannot be modified
volatile int vi = 10;          // May change unexpectedly
mutable int mi;                // Can be modified even in const context
```

### Type Aliases
```cpp
typedef int MyInt;             // C-style
using MyInt = int;             // C++11 style (preferred)
```

### Auto Type Deduction (C++11)
```cpp
auto x = 42;                   // int
auto y = 3.14;                 // double
auto z = "hello";              // const char*
auto& ref = x;                 // int&
auto* ptr = &x;                // int*
```

---

## Header Files

### What Are Header Files?
Header files (`.h` or `.hpp`) contain:
- Function declarations
- Class/struct definitions
- Constants
- Type definitions
- Template code

### Why Use Header Files?
1. **Separation of Interface and Implementation**: Declarations in headers, definitions in `.cpp` files
2. **Reusability**: Include once, use in multiple source files
3. **Compilation Speed**: Only recompile changed files
4. **Organization**: Logical grouping of related code

### Standard Library Headers
```cpp
#include <iostream>            // Input/output streams
#include <vector>              // Dynamic array
#include <string>              // String class
#include <algorithm>           // Algorithms (sort, find, etc.)
#include <cmath>               // Math functions
#include <cstdlib>             // C standard library
#include <memory>              // Smart pointers
#include <fstream>             // File streams
#include <sstream>             // String streams
#include <map>                 // Map container
#include <set>                 // Set container
#include <unordered_map>       // Hash map
#include <thread>              // Threading (C++11)
#include <chrono>              // Time utilities (C++11)
```

### Include Guards (Traditional)
```cpp
#ifndef MY_HEADER_H
#define MY_HEADER_H

// Header content here

#endif // MY_HEADER_H
```

### Include Guards (Modern - C++11)
```cpp
#pragma once

// Header content here
```

### Example Header File (`math_utils.h`)
```cpp
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

// Function declarations
int add(int a, int b);
double multiply(double x, double y);
float power(float base, int exponent);

// Constant
const double PI = 3.14159265359;

// Inline function (definition in header)
inline int subtract(int a, int b) {
    return a - b;
}

#endif // MATH_UTILS_H
```

---

## Multiple Files

### Project Structure Example
```
project/
├── main.cpp          # Entry point
├── math_utils.h      # Header file
├── math_utils.cpp    # Implementation
├── calculator.h      # Another header
└── calculator.cpp    # Another implementation
```

### Example: Multi-File Project

#### `math_utils.h`
```cpp
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

int add(int a, int b);
int subtract(int a, int b);
double multiply(double x, double y);

#endif
```

#### `math_utils.cpp`
```cpp
#include "math_utils.h"

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

double multiply(double x, double y) {
    return x * y;
}
```

#### `main.cpp`
```cpp
#include <iostream>
#include "math_utils.h"

int main() {
    int sum = add(5, 3);
    int diff = subtract(10, 4);
    double product = multiply(2.5, 4.0);
    
    std::cout << "Sum: " << sum << std::endl;
    std::cout << "Difference: " << diff << std::endl;
    std::cout << "Product: " << product << std::endl;
    
    return 0;
}
```

### Include Syntax
```cpp
#include "local_header.h"      // For your own headers (searches current directory first)
#include <standard_header>     // For standard library (searches system paths)
```

---

## Compilation & Running

### Basic Compilation
```bash
# Single file
g++ -std=c++20 main.cpp -o main

# Multiple files
g++ -std=c++20 main.cpp math_utils.cpp -o program

# All .cpp files in directory
g++ -std=c++20 *.cpp -o program
```

### Common Compiler Flags
```bash
# C++ standard
-std=c++11    # C++11
-std=c++14    # C++14
-std=c++17    # C++17
-std=c++20    # C++20 (recommended)

# Warnings
-Wall         # Enable most warnings
-Wextra       # Enable extra warnings
-Werror       # Treat warnings as errors
-Wpedantic    # Strict ISO C++ compliance

# Optimization
-O0           # No optimization (debug)
-O1           # Basic optimization
-O2           # More optimization (release)
-O3           # Maximum optimization
-Os           # Optimize for size

# Debugging
-g            # Include debug symbols
-g3           # Maximum debug information

# Output
-o filename   # Specify output filename

# Include paths
-I/path/to/headers    # Add include directory

# Library paths
-L/path/to/libs       # Add library directory
-llibrary_name        # Link library (e.g., -lm for math)
```

### Complete Compilation Example
```bash
# Debug build
g++ -std=c++20 -Wall -Wextra -g -O0 main.cpp math_utils.cpp -o program_debug

# Release build
g++ -std=c++20 -Wall -Wextra -O2 main.cpp math_utils.cpp -o program_release
```

### Running Programs
```bash
# Execute
./program

# With arguments
./program arg1 arg2 arg3

# Redirect input/output
./program < input.txt > output.txt
```

### Using Clang++ (Alternative Compiler)
```bash
clang++ -std=c++20 main.cpp -o main
```

---

## Functions

### Function Declaration
```cpp
// Declaration (typically in header)
int add(int a, int b);
void printMessage();
double calculate(double x, double y);
```

### Function Definition
```cpp
// Definition (in .cpp file or header if inline)
int add(int a, int b) {
    return a + b;
}

void printMessage() {
    std::cout << "Hello!" << std::endl;
}

double calculate(double x, double y) {
    return x * y + 10.0;
}
```

### Function Parameters

**Parameter passing strategies:**

#### Pass by Value (Copy)
**When to use:** Small, primitive types (int, double, char, bool)
- Creates a copy of the argument
- Original cannot be modified
- Safe but can be expensive for large objects

```cpp
void func1(int x) {
    x = 100;  // Only modifies the copy, original unchanged
}
```

#### Pass by Reference
**When to use:** When you need to modify the original variable
- No copy made (efficient)
- Original can be modified
- Cannot be null (safer than pointer)

```cpp
void func2(int& x) {
    x = 100;  // Modifies the original variable
}
```

#### Pass by Const Reference (Recommended for large objects)
**When to use:** Large objects you don't need to modify (strings, vectors, custom classes)
- No copy made (efficient)
- Original cannot be modified (safe)
- Best of both worlds: efficiency + safety

```cpp
void func3(const std::string& str) {
    // Can read str but not modify it
    // No expensive copy of the string
}
```

#### Pass by Pointer
**When to use:** When you need optional/nullable parameters, or interfacing with C
- Can be null (optional parameter)
- Can be reassigned
- Requires null checks

```cpp
void func4(int* x) {
    if (x != nullptr) {  // Always check!
        *x = 100;  // Modifies original
    }
}
```

#### Default Parameters
**When to use:** When most calls use the same value, but flexibility is needed
- Provides convenience
- Reduces function overloading

```cpp
void func5(int x = 10, double y = 3.14) {
    // x defaults to 10, y to 3.14 if not provided
}
// Can call: func5(), func5(20), or func5(20, 5.0)
```

### Function Overloading

**Overloading allows multiple functions with the same name** but different parameters:
- **Convenience**: Same name for related operations
- **Type safety**: Compiler chooses correct version based on arguments
- **Flexibility**: Different implementations for different types

**Use cases:**
- Same operation on different types (add integers, add doubles)
- Different numbers of parameters
- Providing default behaviors

### Function Overloading
```cpp
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}
```

### Inline Functions

**Inline functions suggest the compiler to insert code directly** at call sites:
- **Performance**: Eliminates function call overhead
- **Small functions**: Best for simple, frequently-called functions
- **Header-only**: Definitions typically in headers
- **Compiler hint**: Compiler may ignore if function is too complex

**Use cases:**
- Small, frequently-called functions
- Getter/setter methods
- Simple utility functions
- When function call overhead matters

```cpp
// In header file
inline int multiply(int a, int b) {
    return a * b;  // Compiler may insert this code directly at call site
}
```

### Lambda Functions (C++11)

**Lambdas are anonymous functions** defined inline:
- **Convenience**: Define small functions where you use them
- **Closures**: Can capture variables from surrounding scope
- **Algorithm compatibility**: Work with STL algorithms (std::for_each, std::sort)
- **Functional programming**: Enable functional style in C++

**Use cases:**
- Callback functions
- STL algorithms (sorting, filtering, transforming)
- Event handlers
- One-time use functions
- Functional programming patterns

### Lambda Functions (C++11)
```cpp
// Basic lambda
auto lambda = [](int x) { return x * 2; };
int result = lambda(5);  // 10

// With capture
int y = 10;
auto lambda2 = [y](int x) { return x + y; };

// Capture by reference
auto lambda3 = [&y](int x) { y = x; };

// Capture all by value
auto lambda4 = [=](int x) { return x + y; };

// Capture all by reference
auto lambda5 = [&](int x) { y = x; };
```

---

## Classes

### Basic Class Declaration
```cpp
// In header file (calculator.h)
#ifndef CALCULATOR_H
#define CALCULATOR_H

class Calculator {
private:
    double result;
    
public:
    // Constructor
    Calculator();
    Calculator(double initial);
    
    // Destructor
    ~Calculator();
    
    // Methods
    void add(double value);
    void subtract(double value);
    double getResult() const;
    
    // Static method
    static double multiply(double a, double b);
};

#endif
```

### Class Implementation
```cpp
// In .cpp file (calculator.cpp)
#include "calculator.h"

// Default constructor
Calculator::Calculator() : result(0.0) {
}

// Parameterized constructor
Calculator::Calculator(double initial) : result(initial) {
}

// Destructor
Calculator::~Calculator() {
    // Cleanup code (if needed)
}

void Calculator::add(double value) {
    result += value;
}

void Calculator::subtract(double value) {
    result -= value;
}

double Calculator::getResult() const {
    return result;
}

double Calculator::multiply(double a, double b) {
    return a * b;
}
```

### Using Classes
```cpp
#include "calculator.h"
#include <iostream>

int main() {
    Calculator calc(10.0);
    calc.add(5.0);
    calc.subtract(3.0);
    
    std::cout << "Result: " << calc.getResult() << std::endl;
    
    // Static method
    double product = Calculator::multiply(4.0, 5.0);
    
    return 0;
}
```

### Access Specifiers
```cpp
class MyClass {
public:      // Accessible from anywhere
    int publicVar;
    
protected:   // Accessible in class and derived classes
    int protectedVar;
    
private:     // Accessible only in this class
    int privateVar;
};
```

### Constructor Initialization List
```cpp
class Point {
private:
    int x, y;
    
public:
    Point(int x, int y) : x(x), y(y) {
        // x and y initialized before constructor body
    }
};
```

### Copy Constructor & Assignment
```cpp
class MyClass {
public:
    // Copy constructor
    MyClass(const MyClass& other) {
        // Copy data from other
    }
    
    // Copy assignment operator
    MyClass& operator=(const MyClass& other) {
        if (this != &other) {
            // Copy data from other
        }
        return *this;
    }
};
```

---

## Structs

**Structs group related data together**:
- **Data organization**: Bundle related variables into one unit
- **Lightweight objects**: Simple data containers without complex behavior
- **C compatibility**: Can be used with C code (POD - Plain Old Data)
- **Public by default**: Unlike classes, members are public by default

**Use cases:**
- Simple data containers (Point, Color, Date, Rectangle)
- C-style data structures
- When you need POD (Plain Old Data) types
- Grouping related constants or configuration
- Lightweight objects that don't need encapsulation

### Basic Struct
```cpp
// Struct (members public by default)
struct Point {
    int x;
    int y;
    
    // Can have methods (C++ feature)
    void print() {
        std::cout << "(" << x << ", " << y << ")" << std::endl;
    }
};

// Usage
Point p;
p.x = 10;
p.y = 20;
p.print();

// Initialization
Point p2 = {5, 15};
Point p3{30, 40};  // C++11 uniform initialization
```

### Struct vs Class
```cpp
// Struct: public by default, typically for data
struct Data {
    int value;
    std::string name;
};

// Class: private by default, typically for objects with behavior
class Object {
    int value;
    std::string name;
public:
    void doSomething();
};
```

### Struct with Methods
```cpp
struct Rectangle {
    double width;
    double height;
    
    double area() const {
        return width * height;
    }
    
    void scale(double factor) {
        width *= factor;
        height *= factor;
    }
};
```

---

## Pointers & References

### What Are Pointers?
**Pointers store the memory address of a variable**, allowing you to:
- Access and modify variables indirectly
- Pass large objects efficiently (without copying)
- Create dynamic data structures (linked lists, trees)
- Return multiple values from functions
- Work with arrays and dynamic memory

**Use cases:**
- When you need to modify a variable from within a function
- When passing large objects (cheaper than copying)
- When you need optional/nullable references
- When building dynamic data structures
- When interfacing with C code or low-level operations

### Pointers
```cpp
int x = 42;
int* ptr = &x;        // Pointer stores address of x

std::cout << *ptr;    // Dereference: gets value at address (42)
std::cout << ptr;     // Prints the address itself (0x...)

*ptr = 100;           // Modify x through pointer (x is now 100)
```

**How it works:**
- `&x` gets the memory address where `x` is stored
- `ptr` holds that address
- `*ptr` dereferences (follows the address) to get/set the value

### References
**References are aliases** - alternative names for the same variable:
- Must be initialized when declared
- Cannot be reassigned to point to another variable
- Automatically dereferenced (no `*` needed)
- Safer than pointers (cannot be null)

**Use cases:**
- Function parameters (avoid copying, guarantee non-null)
- Return values from functions
- Range-based for loops
- Operator overloading

```cpp
int x = 42;
int& ref = x;         // ref is an alias for x (same memory location)

ref = 100;            // Modifies x (they're the same variable)
std::cout << x;       // 100
```

### Pointer vs Reference
```cpp
// Pointer: can be null, can be reassigned
int* ptr = nullptr;   // Can be null (useful for optional values)
ptr = &x;             // Can point to x
ptr = &y;             // Can be reassigned to point to y

// Reference: cannot be null, cannot be reassigned
int& ref = x;         // Must be initialized (cannot be null)
// ref = y;           // This assigns y's value to x, doesn't change ref
                      // ref always refers to x
```

**When to use pointers:**
- Need optional/nullable values
- Need to reassign what it points to
- Need pointer arithmetic
- Interfacing with C APIs

**When to use references:**
- Function parameters (preferred over pointers)
- When you guarantee the value exists
- Operator overloading
- Range-based for loops

### Dynamic Memory (C++ Style)

**Dynamic memory allows allocation at runtime**:
- **Flexibility**: Size determined at runtime, not compile time
- **Lifetime control**: Manage when objects are created/destroyed
- **Large objects**: Allocate objects too large for stack
- **Resource management**: Critical to avoid memory leaks

**⚠️ Important:** See [Memory Management Guide](./C++_MEMORY_MANAGEMENT.md) for comprehensive details.

```cpp
// Smart pointers (preferred, C++11) - automatically manage memory
#include <memory>

// Unique pointer (exclusive ownership) - one owner, automatically deleted
std::unique_ptr<int> ptr = std::make_unique<int>(42);

// Shared pointer (shared ownership) - multiple owners, deleted when last owner gone
std::shared_ptr<int> shared = std::make_shared<int>(42);

// Raw pointers (avoid if possible) - manual memory management
int* raw = new int(42);
delete raw;  // Must manually delete or memory leak!
```

### Arrays and Pointers
```cpp
int arr[5] = {1, 2, 3, 4, 5};
int* ptr = arr;       // Points to first element

std::cout << *ptr;           // 1
std::cout << *(ptr + 1);     // 2
std::cout << ptr[2];         // 3
```

---

## Common Patterns

### Namespaces
**Namespaces organize code and prevent naming conflicts**:
- **Avoid collisions**: Same function/class names in different libraries
- **Logical grouping**: Related code grouped together
- **Scope management**: Control what names are visible
- **Library organization**: Standard library uses `std::` namespace

**Use cases:**
- Organizing your own code into logical modules
- Avoiding conflicts when using multiple libraries
- Creating library interfaces
- Grouping related constants, functions, classes

```cpp
namespace MyNamespace {
    int value = 42;
    void function() { }
}

// Usage - must specify namespace
MyNamespace::value;
MyNamespace::function();

// Using directive - brings all names into current scope (use carefully!)
using namespace MyNamespace;
value;  // Now accessible directly (but can cause conflicts)

// Using declaration - brings specific name into scope (safer)
using MyNamespace::value;
value;  // Can use without namespace prefix
```

**Best practices:**
- Use `std::` prefix instead of `using namespace std;` in headers
- Create namespaces for your own libraries
- Use `using` declarations (not directives) when needed
- Avoid `using namespace` in header files (pollutes global namespace)

### Enums

**Enums define a set of named constants** representing discrete values:
- **Type safety**: Restrict values to a specific set
- **Readability**: Names instead of magic numbers
- **Maintainability**: Change values in one place
- **Self-documenting**: Code expresses intent clearly

**Use cases:**
- State machines (SUCCESS, ERROR, PENDING)
- Options/flags (RED, GREEN, BLUE)
- Configuration values
- Replacing magic numbers with meaningful names

```cpp
// C-style enum (legacy, avoid in new code)
enum Color {
    RED,      // 0
    GREEN,    // 1
    BLUE      // 2
};
Color c = RED;  // Can implicitly convert to int (problem!)

// Scoped enum (C++11, preferred) - type-safe
enum class Status {
    SUCCESS,  // 0
    ERROR,    // 1
    PENDING   // 2
};
Status s = Status::SUCCESS;  // Must use Status:: prefix (type-safe!)
// Cannot implicitly convert to int
```

**Why `enum class` is preferred:**
- **Type safety**: No implicit conversion to int
- **Namespace**: Values scoped to enum name
- **No name collisions**: `Status::SUCCESS` vs `Color::SUCCESS`

### Constants

**Constants prevent modification** and express intent:
- **Immutability**: Values that shouldn't change
- **Type safety**: Compiler enforces const correctness
- **Documentation**: Code expresses "this won't change"
- **Optimization**: Compiler can optimize const values

**Use cases:**
- Configuration values (MAX_SIZE, PI)
- Function parameters you don't want modified
- Member functions that don't modify the object
- Compile-time constants for optimization

```cpp
// Runtime constant
const int MAX_SIZE = 100;  // Value known at runtime, cannot modify

// Compile-time constant (C++11) - evaluated at compile time
constexpr int COMPILE_TIME_CONST = 42;  // Can be used in template parameters
constexpr double PI = 3.14159265359;

// Const member function - promises not to modify object
class MyClass {
    int value;
public:
    int getValue() const {  // const means "this function doesn't modify object"
        return value;       // Can be called on const objects
    }
    
    void setValue(int v) {  // Not const - modifies object
        value = v;
    }
};

// Usage
const MyClass obj;
int x = obj.getValue();  // OK: const function
// obj.setValue(10);     // ERROR: cannot call non-const on const object
```

**`const` vs `constexpr`:**
- `const`: Value cannot be modified (may be computed at runtime)
- `constexpr`: Value computed at compile time (can be used in templates, array sizes)

### Templates
**Templates enable generic programming** - write code that works with multiple types:
- **Compile-time polymorphism**: Different types, same code
- **Type safety**: Compiler checks types at compile time
- **No runtime overhead**: Code is generated at compile time
- **Code reuse**: Write once, use with many types

**Use cases:**
- Container classes (vector, list, map work with any type)
- Algorithms (sort, find work with any comparable type)
- Utility functions (max, min, swap work with any type)
- Avoiding code duplication for similar operations on different types

```cpp
// Function template - works with any type that supports >
template<typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

// Usage - compiler generates specific versions
int m1 = max(10, 20);              // Generates max<int>
double m2 = max(3.14, 2.71);       // Generates max<double>
std::string m3 = max("a", "b");    // Generates max<std::string>

// Class template - container that works with any type
template<typename T>
class Stack {
private:
    T* data;                        // Type T determined when Stack is created
    int size;
public:
    void push(T item);
    T pop();
};

// Usage - compiler creates Stack<int> and Stack<string> classes
Stack<int> intStack;                // Stack of integers
Stack<std::string> stringStack;    // Stack of strings
```

**How it works:**
- Compiler generates specific code for each type used
- `template<typename T>` means "T is a placeholder for any type"
- When you use `Stack<int>`, compiler creates a version where T=int
- All type checking happens at compile time

### Input/Output
```cpp
#include <iostream>
#include <string>

// Output
std::cout << "Hello" << std::endl;
std::cout << "Value: " << 42 << std::endl;

// Input
int x;
std::cin >> x;

std::string line;
std::getline(std::cin, line);
```

### File I/O
```cpp
#include <fstream>

// Write to file
std::ofstream out("output.txt");
out << "Hello, File!" << std::endl;
out.close();

// Read from file
std::ifstream in("input.txt");
std::string line;
while (std::getline(in, line)) {
    std::cout << line << std::endl;
}
in.close();
```

### Vector (Dynamic Array)
```cpp
#include <vector>

std::vector<int> vec;
vec.push_back(1);
vec.push_back(2);
vec.push_back(3);

// Access
vec[0] = 10;
int size = vec.size();

// Iterate
for (int i = 0; i < vec.size(); i++) {
    std::cout << vec[i] << std::endl;
}

// Range-based for loop (C++11)
for (int value : vec) {
    std::cout << value << std::endl;
}
```

### String
```cpp
#include <string>

std::string str = "Hello";
str += " World";
int len = str.length();
char c = str[0];

// Comparison
if (str == "Hello World") { }

// Substring
std::string sub = str.substr(0, 5);  // "Hello"
```

---

## Quick Reference: Compilation Commands

```bash
# Single file, basic
g++ file.cpp -o program

# Single file, C++20, warnings
g++ -std=c++20 -Wall -Wextra file.cpp -o program

# Multiple files
g++ -std=c++20 main.cpp utils.cpp -o program

# All .cpp files in directory
g++ -std=c++20 *.cpp -o program

# Debug build
g++ -std=c++20 -g -O0 file.cpp -o program_debug

# Release build
g++ -std=c++20 -O2 file.cpp -o program_release

# With include directory
g++ -std=c++20 -I./include file.cpp -o program

# With library
g++ -std=c++20 file.cpp -lm -o program
```

---

## Quick Reference: Type Sizes (Typical)

```
char:           1 byte
short:          2 bytes
int:            4 bytes
long:           4 or 8 bytes
long long:      8 bytes
float:          4 bytes
double:         8 bytes
long double:    8 or 16 bytes
bool:           1 byte
pointer:        4 or 8 bytes (depends on system)
```

---

## Tips for Offline Development

1. **Always use include guards** in header files
2. **Keep declarations in headers**, definitions in .cpp files
3. **Use const references** for large objects passed to functions
4. **Prefer smart pointers** over raw pointers
5. **Use -Wall -Wextra** flags to catch errors early
6. **Compile frequently** to catch errors quickly
7. **Use meaningful variable names** - you won't have autocomplete help
8. **Comment complex logic** - you'll thank yourself later

---

**Happy Coding! 🚀**

