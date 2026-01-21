---
name: modern-cpp-refactor
description: "Modern C++ refactoring intelligence. Scans and modernizes legacy C/C++98/11 code to C++17/20 standards. Detects: raw pointers, manual memory, C-style arrays, old containers, manual loops. Replaces with: smart pointers, RAII, std::array/vector, ranges, std::format, structured bindings, auto, constexpr, concepts, std::optional, std::variant. Actions: scan, analyze, refactor, modernize, upgrade, review, check C++ code. Patterns: memory leaks, null checks, iterator loops, string formatting, error handling, ownership semantics."
---

# Modern C++ Refactor - Code Modernization Intelligence

Automated detection and refactoring of legacy C/C++98/11 code to modern C++17/20 standards using smart pointers, ranges, std::format, and contemporary best practices.

## Prerequisites

Check if Python is installed:

```bash
python3 --version || python --version
```

If Python is not installed, install it based on user's OS:

**macOS:**
```bash
brew install python3
```

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt install python3
```

**Windows:**
```powershell
winget install Python.Python.3.12
```

---

## How to Use This Skill

When user requests C++ code refactoring, modernization, or asks to write new C++ code, follow this workflow:

### Step 0: Check Project Coding Standards (CRITICAL)

**ALWAYS check for `.clang-format` file first:**

```bash
# Check if .clang-format exists in project
ls .clang-format || find . -name ".clang-format" -type f
```

If `.clang-format` exists:

- **STRICTLY FOLLOW** the formatting rules defined in it
- Do NOT deviate from the project's established style
- Use the same brace style, indentation, spacing as configured
- After refactoring, format code with: `clang-format -i <file>`

If `.clang-format` does NOT exist:

- Follow Modern C++ best practices from this guide
- Consider creating a `.clang-format` for consistency

**Also check for other project standards:**

- `CMakeLists.txt` or `Makefile` for C++ standard version (C++17/20/23)
- `CONTRIBUTING.md` or `STYLE.md` for coding guidelines
- Existing code patterns in the codebase

### Step 1: Scan for Legacy Patterns

Use the scanner script to detect legacy C++ patterns:

```bash
python3 .claude/skills/modern-cpp-refactor/scripts/scan.py <file_or_directory> [--detailed]
```

This will identify:
- Raw pointers and manual memory management
- C-style arrays and strings
- Old-style casts
- Manual loops vs ranges
- Printf/sprintf vs std::format
- NULL vs nullptr
- typedef vs using

### Step 2: Analyze Code Context

Before refactoring, understand:
- **C++ Standard**: What standard is currently used? (Check CMakeLists.txt, Makefile, compiler flags)
- **Dependencies**: Are there third-party libraries that require specific patterns?
- **Ownership**: Who owns the memory? (unique_ptr, shared_ptr, or value semantics?)
- **Performance**: Is this hot path code? (Consider move semantics, perfect forwarding)

### Step 3: Apply Modern C++ Transformations

Follow the modernization rules below to refactor code systematically.

---

## Modern C++ Transformation Rules

### Memory Management

| Legacy Pattern | Modern C++17/20 | When to Use |
|----------------|-----------------|-------------|
| `new` / `delete` | `std::make_unique<T>()` | Exclusive ownership |
| `new` / `delete` | `std::make_shared<T>()` | Shared ownership |
| Raw pointer parameters | `T&` or `std::span<T>` | Non-owning access |
| `new T[size]` | `std::vector<T>` or `std::array<T, N>` | Dynamic/fixed arrays |
| Manual `delete[]` | Automatic via RAII containers | Always |
| `malloc` / `free` | `std::vector` or smart pointers | C++ code (never use malloc) |

**Example:**
```cpp
// Legacy
int* data = new int[100];
// ... use data ...
delete[] data;

// Modern C++17/20
auto data = std::vector<int>(100);
// ... use data ... (automatic cleanup)

// Or for fixed size:
auto data = std::array<int, 100>{};
```

### Raw Pointers to Smart Pointers

| Legacy Pattern | Modern C++17/20 | Reasoning |
|----------------|-----------------|-----------|
| `Widget* ptr = new Widget();` | `auto ptr = std::make_unique<Widget>();` | Exception-safe, RAII |
| Function returns `T*` (ownership) | Function returns `std::unique_ptr<T>` | Clear ownership transfer |
| Function accepts `T*` (ownership) | Function accepts `std::unique_ptr<T>` | Clear ownership transfer |
| Function accepts `T*` (non-owning) | Function accepts `T&` or `T*` (document!) | Non-null use `T&` |
| `NULL` | `nullptr` | Type-safe null |

**Example:**
```cpp
// Legacy
class Manager {
    Widget* widget;
public:
    Manager() : widget(new Widget()) {}
    ~Manager() { delete widget; }
};

// Modern C++17/20
class Manager {
    std::unique_ptr<Widget> widget;
public:
    Manager() : widget(std::make_unique<Widget>()) {}
    // No destructor needed - automatic cleanup
};
```

### Containers and Arrays

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| `T arr[N]` | `std::array<T, N>` | Bounds checking, STL algorithms |
| `T* arr = new T[n]` | `std::vector<T>` | RAII, dynamic sizing |
| C-string `char*` | `std::string` | Memory-safe, easier manipulation |
| `char buffer[256]` | `std::string` or `std::vector<char>` | No buffer overflow |
| Manual size tracking | `.size()` method | Automatic |

**Example:**
```cpp
// Legacy
char buffer[256];
sprintf(buffer, "Value: %d", value);

// Modern C++20
auto buffer = std::format("Value: {}", value);

// Modern C++17 (without format)
auto buffer = std::string("Value: ") + std::to_string(value);
```

### Loops and Algorithms

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| `for (int i = 0; i < n; i++)` | `for (auto& elem : container)` | Cleaner, less error-prone |
| Iterator loops | Range-based for or ranges | More expressive |
| Manual `find` loop | `std::find`, `std::find_if` | Standard, optimized |
| Manual `sum` loop | `std::accumulate` or ranges | Declarative intent |
| Manual `transform` loop | `std::transform` or ranges::transform | Functional style |

**Example:**
```cpp
// Legacy
std::vector<int> vec = {1, 2, 3, 4, 5};
for (size_t i = 0; i < vec.size(); i++) {
    vec[i] *= 2;
}

// Modern C++17
for (auto& val : vec) {
    val *= 2;
}

// Modern C++20 (ranges)
#include <ranges>
auto doubled = vec | std::views::transform([](int x) { return x * 2; });
```

### Type Declarations

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| Explicit types everywhere | `auto` for obvious types | Less typing, easier refactoring |
| `typedef std::vector<int> IntVec` | `using IntVec = std::vector<int>` | Consistent syntax |
| Type-specific functions | `template<typename T>` with concepts | Type-safe generics |
| `std::pair` with `.first`, `.second` | Structured bindings `auto [a, b]` | Named values |

**Example:**
```cpp
// Legacy
std::map<std::string, int>::iterator it = myMap.find("key");
if (it != myMap.end()) {
    int value = it->second;
}

// Modern C++17
if (auto it = myMap.find("key"); it != myMap.end()) {
    auto [key, value] = *it;
    // Use key and value directly
}

// Modern C++17 (structured binding)
for (const auto& [key, value] : myMap) {
    // Use key and value directly
}
```

### Error Handling

| Legacy Pattern | Modern C++17/20 | When to Use |
|----------------|-----------------|-------------|
| Return `-1` or `nullptr` | `std::optional<T>` | Operation may fail |
| Exception + error code | `std::expected<T, Error>` (C++23) or `std::optional` | Expected failures |
| Multiple return types | `std::variant<T1, T2>` | Type-safe union |
| Boolean + out parameter | Return `std::optional<T>` | Cleaner API |

**Example:**
```cpp
// Legacy
Widget* findWidget(int id) {
    // ... search ...
    if (not_found) return nullptr;
    return widget;
}

// Modern C++17
std::optional<Widget> findWidget(int id) {
    // ... search ...
    if (not_found) return std::nullopt;
    return widget;
}

// Usage:
if (auto widget = findWidget(42)) {
    // Use *widget
}
```

### String Formatting

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| `sprintf`, `snprintf` | `std::format` (C++20) | Type-safe, no buffer overflow |
| `std::stringstream` | `std::format` | Simpler, faster |
| Manual string concat | `std::format` or `fmt::format` | Readable |

**Example:**
```cpp
// Legacy
char buffer[256];
sprintf(buffer, "Name: %s, Age: %d", name.c_str(), age);

// Modern C++20
auto result = std::format("Name: {}, Age: {}", name, age);

// Alternative (C++17 with fmt library):
auto result = fmt::format("Name: {}, Age: {}", name, age);
```

### Casts

| Legacy Pattern | Modern C++17/20 | Safety |
|----------------|-----------------|--------|
| C-style cast `(Type)value` | `static_cast<Type>(value)` | Explicit intent |
| `(Type)ptr` for pointers | `dynamic_cast<Type*>(ptr)` | Runtime type check |
| Const removal `(Type)` | `const_cast<Type>(value)` | Explicit const violation |
| `reinterpret_cast` | Avoid or document heavily | Dangerous |

### Initialization

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| `Type var = Type(args)` | `Type var{args}` | Uniform initialization |
| `Type var;` (uninitialized) | `Type var{}` | Zero-initialized |
| Copy initialization | Direct initialization `{}` | No implicit conversions |

**Example:**
```cpp
// Legacy
std::vector<int> vec = std::vector<int>(10, 5);

// Modern C++17
auto vec = std::vector<int>(10, 5);
// Or even better:
std::vector<int> vec(10, 5);
```

### Const Correctness

| Rule | Modern C++ Standard |
|------|---------------------|
| Mark functions `const` if they don't modify | Always |
| Use `const&` for read-only parameters | For non-primitive types |
| Use `constexpr` for compile-time constants | C++17/20 |
| Use `if constexpr` for compile-time branches | C++17 |

**Example:**
```cpp
// Legacy
int getValue() { return value; }

// Modern C++17
int getValue() const { return value; }
constexpr int getMax() const { return 100; }
```

### Move Semantics

| Legacy Pattern | Modern C++17/20 | Benefits |
|----------------|-----------------|----------|
| Pass by value (copy) | Pass by `const&` | No unnecessary copy |
| Return large objects | Return by value (RVO/NRVO) | Compiler optimization |
| Explicit copy | `std::move` for transfer | Efficient |

**Example:**
```cpp
// Modern C++17
class Widget {
    std::vector<int> data;
public:
    // Move constructor
    Widget(Widget&& other) noexcept
        : data(std::move(other.data)) {}

    // Move assignment
    Widget& operator=(Widget&& other) noexcept {
        data = std::move(other.data);
        return *this;
    }
};
```

### Concepts (C++20)

| Legacy Pattern | Modern C++20 | Benefits |
|----------------|--------------|----------|
| Template + SFINAE | Concepts | Clear constraints, better errors |
| `typename` everywhere | Concepts with semantic names | Self-documenting |

**Example:**
```cpp
// Legacy
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
add(T a, T b) { return a + b; }

// Modern C++20
template<std::integral T>
T add(T a, T b) { return a + b; }

// Or with concept:
std::integral auto add(std::integral auto a, std::integral auto b) {
    return a + b;
}
```

---

## Code Scanning Categories

The scanner will check for these categories:

### 1. Memory Safety Issues
- Raw `new` / `delete`
- Manual array allocation
- Missing `delete` (potential leaks)
- `malloc` / `free` in C++ code

### 2. Type Safety Issues
- C-style casts
- `NULL` instead of `nullptr`
- Missing `const` qualifiers
- Implicit conversions

### 3. Modernization Opportunities
- `typedef` instead of `using`
- Manual loops vs ranges
- `std::bind` vs lambdas
- Raw function pointers vs `std::function`

### 4. String and I/O Issues
- C-style strings (`char*`, `char[]`)
- `sprintf` family
- `printf` vs `std::format`

### 5. Concurrency (if applicable)
- Raw `std::thread` without RAII wrapper
- Manual locking vs `std::lock_guard`
- Missing `std::atomic` for shared data

---

## Pre-Refactor Checklist

Before modernizing code, verify:

- [ ] Identify the target C++ standard (C++17 or C++20)
- [ ] Check if compiler supports required features
- [ ] Understand existing ownership semantics
- [ ] Identify performance-critical sections
- [ ] Check for third-party library constraints
- [ ] Ensure comprehensive test coverage exists
- [ ] Plan incremental refactoring (not all at once)

## Post-Refactor Checklist

After modernizing code, verify:

- [ ] All tests still pass
- [ ] No memory leaks (use valgrind/sanitizers)
- [ ] Performance is acceptable (profile if needed)
- [ ] Code compiles with `-Wall -Wextra -Werror`
- [ ] Smart pointers have correct ownership semantics
- [ ] Move semantics applied where beneficial
- [ ] `const` correctness maintained
- [ ] No raw `new` / `delete` remaining (except special cases)

---

## Example Workflow

**User request:** "Modernize this C++ class to use C++17/20 features"

**AI should:**

1. **Scan the code:**
```bash
python3 .claude/skills/modern-cpp-refactor/scripts/scan.py src/MyClass.cpp --detailed
```

2. **Analyze findings:**
   - Identify legacy patterns (raw pointers, manual loops, etc.)
   - Understand ownership semantics
   - Check for performance implications

3. **Refactor systematically:**
   - Replace raw pointers with smart pointers
   - Convert manual loops to range-based or algorithms
   - Use `auto` for obvious types
   - Apply structured bindings where applicable
   - Replace `sprintf` with `std::format` (C++20) or alternatives
   - Add `constexpr` and `const` where appropriate

4. **Verify:**
   - Compile with modern C++ flags
   - Run tests
   - Check for memory leaks
   - Review performance

---

## Common Anti-Patterns to Avoid

### Don't Over-Use Smart Pointers
```cpp
// Bad: Using unique_ptr for simple value
std::unique_ptr<int> value = std::make_unique<int>(42);

// Good: Just use a value
int value = 42;
```

### Don't Use Shared Ptr Everywhere
```cpp
// Bad: Shared ownership without need
std::shared_ptr<Widget> widget = std::make_shared<Widget>();

// Good: Unique ownership when possible
auto widget = std::make_unique<Widget>();
```

### Don't Ignore Move Semantics
```cpp
// Bad: Unnecessary copy
std::vector<int> getData() {
    std::vector<int> result = computeData();
    return result; // Copy
}

// Good: Move semantics (or RVO)
std::vector<int> getData() {
    return computeData(); // Move or RVO
}
```

---

## Quick Reference: Modern C++ Features by Standard

### C++17 Must-Use Features
- Structured bindings: `auto [a, b] = pair;`
- `if` / `switch` with initializer: `if (auto x = foo(); x > 0)`
- `std::optional<T>` for maybe-values
- `std::variant<T1, T2>` for type-safe unions
- `std::string_view` for non-owning string refs
- Fold expressions in templates
- Class template argument deduction (CTAD)

### C++20 Must-Use Features
- `std::format` for string formatting
- Ranges and views
- Concepts for template constraints
- `std::span<T>` for array views
- Three-way comparison operator `<=>`
- Designated initializers: `Point{.x=1, .y=2}`
- `consteval` for compile-time evaluation

---

## When NOT to Modernize

Sometimes legacy patterns are necessary:

1. **Interfacing with C APIs**: May require raw pointers
2. **Performance-critical code**: Profile before/after
3. **Third-party library constraints**: API may require specific types
4. **Incremental migration**: Don't refactor everything at once
5. **Platform limitations**: Embedded systems may lack std library

---

## Additional Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Modern C++ Features](https://github.com/AnthonyCalandra/modern-cpp-features)
- [C++17 in Detail](https://leanpub.com/cpp17)
- [C++20 Ranges](https://en.cppreference.com/w/cpp/ranges)

---

## Default Behavior

When user asks you to write NEW C++ code (not just refactor):

**STEP 0 - ALWAYS CHECK FIRST:**

- **Check for `.clang-format` file** - If it exists, STRICTLY follow its formatting rules

**THEN follow these modern C++ standards:**

1. **Always use C++17/20 standards by default**
2. **Never use raw `new` / `delete`** - use smart pointers or RAII containers
3. **Prefer `std::format` (C++20)** or `fmt` library over `sprintf`
4. **Use `auto`** for obvious types
5. **Use range-based for** instead of manual index loops
6. **Use `std::optional`** for maybe-values
7. **Use structured bindings** for pairs/tuples/structs
8. **Mark functions `const` and `constexpr`** when applicable
9. **Use `nullptr`** never `NULL`
10. **Use `using`** never `typedef`
11. **Respect project's `.clang-format`** for all code formatting decisions

This ensures all generated code follows both modern C++ best practices AND the project's established coding standards.
