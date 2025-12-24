# C++ Memory Management Guide

## Table of Contents
1. [Memory Fundamentals](#memory-fundamentals)
2. [Stack vs Heap Memory](#stack-vs-heap-memory)
3. [Automatic Memory Management](#automatic-memory-management)
4. [Dynamic Memory Management](#dynamic-memory-management)
5. [Smart Pointers](#smart-pointers)
6. [RAII (Resource Acquisition Is Initialization)](#raii-resource-acquisition-is-initialization)
7. [Common Memory Errors](#common-memory-errors)
8. [Best Practices](#best-practices)
9. [Memory Management Patterns](#memory-management-patterns)

---

## Memory Fundamentals

### What is Memory Management?
Memory management is the process of controlling and coordinating how programs use computer memory. In C++, you have direct control over memory, which gives you:
- **Performance**: Direct memory access is fast
- **Flexibility**: Allocate exactly what you need
- **Responsibility**: You must manage memory correctly or face crashes/leaks

### Why Memory Management Matters
- **Memory leaks**: Allocated memory never freed → program uses more and more memory
- **Dangling pointers**: Accessing memory that's been freed → crashes
- **Double deletion**: Freeing memory twice → crashes
- **Buffer overflows**: Writing past allocated memory → security vulnerabilities

---

## Stack vs Heap Memory

### Stack Memory
**Stack** is automatic, fast, and limited:
- **Automatic allocation/deallocation**: Created when variable enters scope, destroyed when it leaves
- **Fast**: Simple pointer increment/decrement
- **Limited size**: Typically 1-8 MB (platform dependent)
- **LIFO**: Last In, First Out (like a stack of plates)
- **Local variables**: Function parameters, local variables

```cpp
void function() {
    int x = 42;           // Allocated on stack
    double y = 3.14;      // Allocated on stack
    // x and y automatically destroyed when function returns
}
```

**Characteristics:**
- Very fast allocation/deallocation
- Size must be known at compile time
- Automatic cleanup (no manual management)
- Limited size (stack overflow if too large)

### Heap Memory
**Heap** is manual, slower, but flexible:
- **Manual allocation/deallocation**: You control when memory is allocated/freed
- **Slower**: More complex allocation algorithm
- **Large size**: Limited by system RAM
- **Flexible**: Size determined at runtime
- **Dynamic allocation**: Use `new`/`delete` or smart pointers

```cpp
void function() {
    int* ptr = new int(42);  // Allocated on heap
    // Must manually delete: delete ptr;
}
```

**Characteristics:**
- Slower than stack
- Size determined at runtime
- Manual cleanup required (or use smart pointers)
- Can allocate large amounts
- Can outlive the function that created it

### Comparison Table

| Feature | Stack | Heap |
|---------|-------|------|
| Speed | Very Fast | Slower |
| Size | Limited (~1-8 MB) | Large (system RAM) |
| Lifetime | Scope-based | Manual control |
| Management | Automatic | Manual |
| Fragmentation | None | Possible |
| When to use | Small, known size | Large, runtime size |

---

## Automatic Memory Management

### Stack-Based Variables
Variables allocated on the stack are automatically managed:

```cpp
void example() {
    int x = 10;                    // Stack: auto-allocated
    std::string str = "Hello";     // Stack: auto-allocated
    std::vector<int> vec(100);     // Stack: vec object auto-allocated
                                   // (but internal array may be on heap)
    
    // All automatically destroyed when function returns
}
```

### Automatic Cleanup
```cpp
class Resource {
public:
    Resource() { std::cout << "Acquired\n"; }
    ~Resource() { std::cout << "Released\n"; }
};

void function() {
    Resource r;  // Constructor called
    // ... use resource ...
}  // Destructor automatically called when r goes out of scope
```

**Key Point**: Stack-based objects are automatically destroyed when they go out of scope. This is the foundation of RAII.

---

## Dynamic Memory Management

### Raw Pointers: `new` and `delete`

#### Single Object Allocation
```cpp
// Allocate
int* ptr = new int(42);        // Allocates memory, initializes to 42
int* ptr2 = new int;           // Allocates memory, uninitialized

// Use
*ptr = 100;
std::cout << *ptr << std::endl;

// Deallocate (CRITICAL: must match every new with delete)
delete ptr;                    // Frees the memory
ptr = nullptr;                 // Good practice: set to null after delete
```

#### Array Allocation
```cpp
// Allocate array
int* arr = new int[10];        // Allocates array of 10 integers

// Use
for (int i = 0; i < 10; i++) {
    arr[i] = i;
}

// Deallocate array (CRITICAL: use delete[], not delete)
delete[] arr;                  // Frees the entire array
arr = nullptr;
```

#### Common Mistakes
```cpp
// ❌ WRONG: Mismatched new/delete
int* ptr = new int[10];
delete ptr;                    // WRONG! Should be delete[]

// ❌ WRONG: Double deletion
int* ptr = new int(42);
delete ptr;
delete ptr;                    // CRASH! Already deleted

// ❌ WRONG: Memory leak
int* ptr = new int(42);
// Forgot to delete ptr - memory leak!

// ❌ WRONG: Dangling pointer
int* ptr = new int(42);
delete ptr;
*ptr = 100;                    // CRASH! Accessing freed memory
```

### Why Raw Pointers Are Dangerous
1. **Memory leaks**: Easy to forget `delete`
2. **Double deletion**: Can delete same pointer twice
3. **Dangling pointers**: Use pointer after deletion
4. **Exception safety**: If exception thrown, `delete` might not execute
5. **Ownership unclear**: Who owns the memory? Who should delete it?

**Solution**: Use smart pointers (see below)!

---

## Smart Pointers

**Smart pointers automatically manage memory** - they delete the object when it's no longer needed. Introduced in C++11.

### `std::unique_ptr` - Exclusive Ownership

**One owner, automatically deleted when unique_ptr is destroyed.**

```cpp
#include <memory>

// Creation
std::unique_ptr<int> ptr = std::make_unique<int>(42);
// or
std::unique_ptr<int> ptr2(new int(42));  // Less preferred

// Use like regular pointer
*ptr = 100;
std::cout << *ptr << std::endl;

// Automatically deleted when ptr goes out of scope
// No manual delete needed!
```

**Key Features:**
- **Exclusive ownership**: Only one `unique_ptr` can own the object
- **Automatic deletion**: Destructor automatically calls `delete`
- **Move semantics**: Can transfer ownership (not copyable)
- **Exception safe**: Even if exception thrown, memory is freed

**Transferring Ownership:**
```cpp
std::unique_ptr<int> ptr1 = std::make_unique<int>(42);
std::unique_ptr<int> ptr2 = std::move(ptr1);  // Ownership transferred
// ptr1 is now nullptr
// ptr2 owns the memory
```

**Array Support:**
```cpp
std::unique_ptr<int[]> arr = std::make_unique<int[]>(10);
arr[0] = 42;  // Can use array syntax
// Automatically uses delete[] when destroyed
```

### `std::shared_ptr` - Shared Ownership

**Multiple owners, deleted when last owner is destroyed.**

```cpp
#include <memory>

// Creation
std::shared_ptr<int> ptr1 = std::make_shared<int>(42);
std::shared_ptr<int> ptr2 = ptr1;  // Both share ownership

// Reference counting
std::cout << ptr1.use_count() << std::endl;  // 2 (ptr1 and ptr2)

// When ptr1 goes out of scope, object still exists (ptr2 owns it)
// When ptr2 goes out of scope, object is deleted (last owner)
```

**Key Features:**
- **Shared ownership**: Multiple `shared_ptr` can point to same object
- **Reference counting**: Tracks how many `shared_ptr` point to object
- **Automatic deletion**: Deleted when last `shared_ptr` is destroyed
- **Thread-safe**: Reference counting is thread-safe (C++11)

**Use Cases:**
- When multiple objects need to share the same resource
- When ownership is unclear or shared
- In data structures (trees, graphs)

**Example:**
```cpp
class Node {
public:
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    int data;
    
    Node(int d) : data(d) {}
};

// Tree nodes can share references
std::shared_ptr<Node> root = std::make_shared<Node>(10);
root->left = std::make_shared<Node>(5);
root->right = std::make_shared<Node>(15);
// When root is destroyed, children are also destroyed (if no other references)
```

### `std::weak_ptr` - Non-Owning Reference

**Non-owning reference to object managed by `shared_ptr`.**

```cpp
#include <memory>

std::shared_ptr<int> shared = std::make_shared<int>(42);
std::weak_ptr<int> weak = shared;  // Doesn't increase reference count

// Check if object still exists
if (auto locked = weak.lock()) {  // Try to get shared_ptr
    std::cout << *locked << std::endl;  // Object still exists
} else {
    // Object has been deleted
}

// Use case: Break circular references
```

**Key Features:**
- **Non-owning**: Doesn't keep object alive
- **Break cycles**: Prevents circular reference problems with `shared_ptr`
- **Safe access**: Must check if object exists before use

**Circular Reference Problem:**
```cpp
// ❌ PROBLEM: Circular reference
struct Parent {
    std::shared_ptr<Child> child;
};

struct Child {
    std::shared_ptr<Parent> parent;  // Circular reference!
    // Neither will be deleted
};

// ✅ SOLUTION: Use weak_ptr
struct Child {
    std::weak_ptr<Parent> parent;  // Doesn't keep parent alive
};
```

### Smart Pointer Comparison

| Feature | `unique_ptr` | `shared_ptr` | `weak_ptr` |
|---------|--------------|--------------|------------|
| Ownership | Exclusive | Shared | None |
| Copyable | No (move only) | Yes | Yes |
| Reference count | N/A | Yes | Yes (but doesn't own) |
| Performance | Fastest | Slower (ref counting) | Fastest |
| Use when | Single owner | Multiple owners | Break cycles |

### Creating Smart Pointers

**Preferred: `make_unique` and `make_shared`**
```cpp
// ✅ PREFERRED: Exception safe, more efficient
auto ptr1 = std::make_unique<int>(42);
auto ptr2 = std::make_shared<int>(42);

// ❌ Less preferred: Two allocations (object + control block)
std::shared_ptr<int> ptr3(new int(42));
```

**Why `make_*` is better:**
- **Exception safe**: If constructor throws, no memory leak
- **More efficient**: `make_shared` combines object and control block in one allocation
- **Cleaner syntax**: No need for `new`

---

## RAII (Resource Acquisition Is Initialization)

**RAII is the fundamental C++ memory management principle**: Resources are acquired in constructors and released in destructors.

### The Principle
1. **Acquire resource in constructor**: Allocate memory, open file, lock mutex
2. **Use resource**: Do your work
3. **Release resource in destructor**: Automatically called when object goes out of scope

### Why RAII Works
- Destructors are **always** called (even if exception thrown)
- Stack unwinding ensures cleanup
- No manual resource management needed

### Example: File Management
```cpp
#include <fstream>

// Without RAII (manual management)
void bad_example() {
    std::ofstream file("data.txt");
    // ... write data ...
    file.close();  // Must remember to close!
    // What if exception thrown? File might not close!
}

// With RAII (automatic)
void good_example() {
    std::ofstream file("data.txt");  // File opened in constructor
    // ... write data ...
}  // File automatically closed in destructor (even if exception thrown)
```

### Example: Custom RAII Class
```cpp
class MemoryManager {
private:
    int* data;
    
public:
    MemoryManager(size_t size) : data(new int[size]) {
        // Resource acquired in constructor
        std::cout << "Memory allocated\n";
    }
    
    ~MemoryManager() {
        delete[] data;  // Resource released in destructor
        std::cout << "Memory freed\n";
    }
    
    // Delete copy constructor/assignment (or implement them properly)
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    int* get() { return data; }
};

void function() {
    MemoryManager mem(100);  // Memory allocated
    // ... use mem.get() ...
}  // Memory automatically freed when mem goes out of scope
```

### RAII with Smart Pointers
Smart pointers are the perfect example of RAII:
```cpp
void function() {
    std::unique_ptr<int> ptr = std::make_unique<int>(42);
    // Memory acquired
    // ... use ptr ...
}  // Memory automatically freed in unique_ptr destructor
```

---

## Common Memory Errors

### 1. Memory Leak
**Allocating memory but never freeing it.**

```cpp
// ❌ MEMORY LEAK
void leak() {
    int* ptr = new int(42);
    // Forgot delete ptr;
    // Memory is lost forever (until program ends)
}

// ✅ FIXED: Use smart pointer
void no_leak() {
    auto ptr = std::make_unique<int>(42);
    // Automatically deleted
}
```

**Symptoms:**
- Program uses more and more memory over time
- System slows down
- Eventually runs out of memory

### 2. Dangling Pointer
**Using a pointer after the memory has been freed.**

```cpp
// ❌ DANGLING POINTER
int* get_ptr() {
    int x = 42;
    return &x;  // Returning address of local variable
}  // x is destroyed here

void use_ptr() {
    int* ptr = get_ptr();
    std::cout << *ptr;  // CRASH! x no longer exists
}

// ✅ FIXED: Return by value or use smart pointer
std::unique_ptr<int> get_ptr() {
    return std::make_unique<int>(42);
}
```

**Symptoms:**
- Crashes or undefined behavior
- Sometimes works (if memory not reused yet)
- Hard to debug

### 3. Double Deletion
**Deleting the same memory twice.**

```cpp
// ❌ DOUBLE DELETION
int* ptr = new int(42);
delete ptr;
delete ptr;  // CRASH! Already deleted

// ✅ FIXED: Set to nullptr after delete
int* ptr = new int(42);
delete ptr;
ptr = nullptr;
delete ptr;  // Safe: deleting nullptr does nothing

// ✅ BETTER: Use smart pointer
auto ptr = std::make_unique<int>(42);
// Can't double delete - only one owner
```

**Symptoms:**
- Immediate crash
- Heap corruption
- Undefined behavior

### 4. Buffer Overflow
**Writing past the end of allocated memory.**

```cpp
// ❌ BUFFER OVERFLOW
int* arr = new int[10];
arr[10] = 42;  // Out of bounds! (valid indices: 0-9)
delete[] arr;

// ✅ FIXED: Check bounds
int* arr = new int[10];
if (index < 10) {
    arr[index] = 42;
}

// ✅ BETTER: Use std::vector
std::vector<int> vec(10);
vec.at(10) = 42;  // Throws exception if out of bounds
```

**Symptoms:**
- Corrupted memory
- Crashes
- Security vulnerabilities

### 5. Use After Free
**Accessing memory after it's been deleted.**

```cpp
// ❌ USE AFTER FREE
int* ptr = new int(42);
delete ptr;
*ptr = 100;  // CRASH! Memory no longer valid

// ✅ FIXED: Set to nullptr
int* ptr = new int(42);
delete ptr;
ptr = nullptr;
if (ptr != nullptr) {  // Check before use
    *ptr = 100;
}

// ✅ BETTER: Use smart pointer
auto ptr = std::make_unique<int>(42);
// Can't use after free - automatically managed
```

### 6. Mismatched new/delete
**Using `delete` instead of `delete[]` or vice versa.**

```cpp
// ❌ MISMATCHED
int* arr = new int[10];
delete arr;  // WRONG! Should be delete[]

// ✅ CORRECT
int* arr = new int[10];
delete[] arr;

// ✅ BETTER: Use smart pointer
auto arr = std::make_unique<int[]>(10);
// Automatically uses delete[]
```

---

## Best Practices

### 1. Prefer Stack Over Heap
```cpp
// ✅ GOOD: Stack allocation (when possible)
void good() {
    int x = 42;
    std::string str = "Hello";
    std::vector<int> vec(100);
    // All automatically managed
}

// ❌ AVOID: Unnecessary heap allocation
void bad() {
    int* x = new int(42);
    delete x;  // Unnecessary complexity
}
```

### 2. Use Smart Pointers Instead of Raw Pointers
```cpp
// ❌ AVOID: Raw pointers
void bad() {
    int* ptr = new int(42);
    // ... use ptr ...
    delete ptr;  // Easy to forget
}

// ✅ GOOD: Smart pointers
void good() {
    auto ptr = std::make_unique<int>(42);
    // ... use ptr ...
    // Automatically deleted
}
```

### 3. Use `make_unique` and `make_shared`
```cpp
// ✅ GOOD
auto ptr1 = std::make_unique<int>(42);
auto ptr2 = std::make_shared<int>(42);

// ❌ AVOID (unless necessary)
std::unique_ptr<int> ptr3(new int(42));
std::shared_ptr<int> ptr4(new int(42));
```

### 4. Prefer `unique_ptr` Over `shared_ptr`
```cpp
// ✅ GOOD: Use unique_ptr when single owner
auto ptr = std::make_unique<MyClass>();

// ❌ AVOID: shared_ptr when not needed
auto ptr = std::make_shared<MyClass>();  // Unnecessary overhead
```

### 5. Use `const` References for Large Objects
```cpp
// ✅ GOOD: No copy, no modification
void process(const std::vector<int>& vec) {
    // Efficient: no copy, safe: can't modify
}

// ❌ AVOID: Unnecessary copy
void process(std::vector<int> vec) {
    // Expensive: entire vector copied
}
```

### 6. Initialize Pointers
```cpp
// ✅ GOOD: Initialize to nullptr
int* ptr = nullptr;
if (ptr != nullptr) {
    *ptr = 42;
}

// ❌ AVOID: Uninitialized pointer
int* ptr;  // Contains garbage
*ptr = 42;  // CRASH!
```

### 7. Set Pointers to nullptr After Delete
```cpp
// ✅ GOOD
int* ptr = new int(42);
delete ptr;
ptr = nullptr;  // Safe to check/delete again

// ❌ AVOID
int* ptr = new int(42);
delete ptr;
// ptr still points to deleted memory (dangling pointer)
```

### 8. Use RAII for All Resources
```cpp
// ✅ GOOD: RAII class
class FileHandler {
    std::ofstream file;
public:
    FileHandler(const std::string& name) : file(name) {}
    // File automatically closed in destructor
};

// ❌ AVOID: Manual resource management
void bad() {
    FILE* f = fopen("data.txt", "w");
    // ... use file ...
    fclose(f);  // Must remember!
}
```

### 9. Check for nullptr Before Use
```cpp
// ✅ GOOD
int* ptr = get_pointer();
if (ptr != nullptr) {
    *ptr = 42;
}

// ❌ AVOID
int* ptr = get_pointer();
*ptr = 42;  // Might crash if ptr is nullptr
```

### 10. Use Containers Instead of Raw Arrays
```cpp
// ✅ GOOD: std::vector (automatic memory management)
std::vector<int> vec(100);
vec[50] = 42;

// ❌ AVOID: Raw arrays (manual management)
int* arr = new int[100];
arr[50] = 42;
delete[] arr;  // Must remember!
```

---

## Memory Management Patterns

### Pattern 1: Factory Function Returning Smart Pointer
```cpp
std::unique_ptr<MyClass> create_object() {
    return std::make_unique<MyClass>();
}

// Usage
auto obj = create_object();  // Ownership transferred to caller
```

### Pattern 2: RAII Wrapper for C Resources
```cpp
class FileRAII {
    FILE* file;
public:
    FileRAII(const char* name) : file(fopen(name, "r")) {
        if (!file) throw std::runtime_error("Cannot open file");
    }
    
    ~FileRAII() {
        if (file) fclose(file);
    }
    
    FILE* get() { return file; }
    
    // Delete copy, allow move
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;
    FileRAII(FileRAII&& other) : file(other.file) {
        other.file = nullptr;
    }
};
```

### Pattern 3: Pimpl (Pointer to Implementation)
```cpp
// Header file
class MyClass {
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;  // Hides implementation details
    
public:
    MyClass();
    ~MyClass();  // Must be defined (needs to see Impl destructor)
    void doSomething();
};

// Implementation file
class MyClass::Impl {
    // Implementation details
};

MyClass::MyClass() : pImpl(std::make_unique<Impl>()) {}
MyClass::~MyClass() = default;  // unique_ptr handles deletion
```

### Pattern 4: Shared Ownership for Data Structures
```cpp
class Node {
public:
    int data;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    
    Node(int d) : data(d) {}
};

// Tree automatically cleaned up when root is destroyed
std::shared_ptr<Node> build_tree() {
    auto root = std::make_shared<Node>(10);
    root->left = std::make_shared<Node>(5);
    root->right = std::make_shared<Node>(15);
    return root;
}
```

---

## Quick Reference: Memory Management Rules

### The Rule of Three/Five/Zero
- **Rule of Three**: If you define destructor, copy constructor, or copy assignment, define all three
- **Rule of Five** (C++11): Add move constructor and move assignment
- **Rule of Zero**: Prefer to use smart pointers/containers so you don't need to define any

### Ownership Guidelines
1. **Single owner?** → Use `unique_ptr`
2. **Multiple owners?** → Use `shared_ptr`
3. **No ownership?** → Use raw pointer or reference
4. **Break cycles?** → Use `weak_ptr`

### When to Use Each
- **Stack**: Small objects, known size, automatic cleanup
- **`unique_ptr`**: Single owner, dynamic allocation
- **`shared_ptr`**: Shared ownership, complex lifetimes
- **`weak_ptr`**: Break circular references, observe without owning
- **Raw pointer**: When you don't own the object (observing, non-owning)

---

## Summary

1. **Prefer stack allocation** when possible
2. **Use smart pointers** instead of raw `new`/`delete`
3. **Prefer `unique_ptr`** over `shared_ptr` when possible
4. **Use `make_unique`/`make_shared`** for creation
5. **Follow RAII**: Acquire in constructor, release in destructor
6. **Never use raw `new`/`delete`** in modern C++ (use smart pointers)
7. **Use containers** (`vector`, `string`) instead of raw arrays
8. **Check for nullptr** before using raw pointers
9. **Set pointers to nullptr** after delete (if using raw pointers)
10. **Understand ownership**: Who owns the memory? Who deletes it?

**Remember**: In modern C++ (C++11+), you should rarely need `new`/`delete`. Use smart pointers and containers instead!

---

**Happy Memory Management! 🧠💾**

