# Nova Language — Official Documentation

> **Nova** is a compiled, statically typed language that compiles directly to native code. Familiar syntax, C-level performance.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Data Types](#2-data-types)
3. [Variables](#3-variables)
4. [Operators](#4-operators)
5. [Control Flow](#5-control-flow)
6. [Loops](#6-loops)
7. [Functions](#7-functions)
8. [Arrays](#8-arrays)
9. [Print](#9-print)
10. [Comments](#10-comments)
11. [Compiling with Nova](#11-compiling-with-nova)
12. [Full Examples](#12-full-examples)

---

## 1. Getting Started

### Installation

Download the Nova compiler, and follow the instructions.

### Hello, World!

Create a file `hello.npp`:

```nova
void main() {
    print("Hello, World!");
}
```

Compile and run:

```bash
./n++ hello.npp
./hello
```

Output:
```
Hello, World!
```

### Program Structure

A Nova program is a sequence of top-level declarations: variables, arrays, and functions. Execution starts at the `main` function.

```nova
// global declarations go here

int main() {
    // main code
    return 0;
}
```

---

## 2. Data Types

Nova has four primitive types:

| Type     | Description                      | Example          |
|----------|----------------------------------|------------------|
| `int`    | 32-bit signed integer            | `42`, `-7`, `0`  |
| `float`  | 32-bit floating point            | `3.14`, `-0.5`   |
| `string` | String literal                   | `"hello, world"` |
| `void`   | No value (functions only)        | —                |

---

## 3. Variables

### Declaration

Variables require an explicit type. They can be declared with or without an initial value.

```nova
int x;
int y = 10;
float pi = 3.14;
string name = "Nova";
```

> Uninitialized variables default to `0` (int/float) or null (string).

### Scope

- **Global**: declared outside functions. Accessible anywhere in the program.
- **Local**: declared inside functions or blocks. Only exist while the block is active.

```nova
int counter = 0;   // global

int main() {
    int local = 5;  // local — only exists inside main
    return 0;
}
```

### Assignment

```nova
x = 42;
```

### Compound Assignment

```nova
x += 5;   // x = x + 5
x -= 3;   // x = x - 3
x *= 2;   // x = x * 2
x /= 4;   // x = x / 4
```

### Increment and Decrement

```nova
x++;   // x = x + 1
x--;   // x = x - 1
```

---

## 4. Operators

### Arithmetic

| Operator | Operation      | Example     |
|----------|----------------|-------------|
| `+`      | Addition       | `a + b`     |
| `-`      | Subtraction    | `a - b`     |
| `*`      | Multiplication | `a * b`     |
| `/`      | Division       | `a / b`     |

### Comparison

| Operator | Meaning           |
|----------|-------------------|
| `==`     | Equal             |
| `!=`     | Not equal         |
| `<`      | Less than         |
| `>`      | Greater than      |
| `<=`     | Less or equal     |
| `>=`     | Greater or equal  |

### Logical

| Operator | Meaning    | Example              |
|----------|------------|----------------------|
| `&&`     | Logical AND | `a > 0 && b > 0`   |
| `\|\|`   | Logical OR  | `a == 0 \|\| b == 0` |
| `!`      | Logical NOT | `!active`          |

> `&&` and `||` use **short-circuit evaluation**: if the result is already determined by the left side, the right side is not evaluated.

### Precedence (highest to lowest)

1. `!` (unary)
2. `*`, `/`
3. `+`, `-`
4. `==`, `!=`, `<`, `>`, `<=`, `>=`
5. `&&`
6. `||`

Use parentheses to control evaluation order:

```nova
int r = (a + b) * (c - d);
if(x > 0 && (y == 1 || z == 2)) then { ... }
```

---

## 5. Control Flow

### if / else

The syntax requires the `then` keyword before the block:

```nova
if(condition) then {
    // executed if true
}
```

With `else`:

```nova
if(x > 0) then {
    print("positive");
} else {
    print("not positive");
}
```

### else if

Chain as many conditions as needed:

```nova
if(score >= 90) then {
    print("A");
} else if(score >= 80) then {
    print("B");
} else if(score >= 70) then {
    print("C");
} else {
    print("failed");
}
```

---

## 6. Loops

### while

Executes while the condition is true:

```nova
int i = 0;
while(i < 10) {
    print(i);
    i++;
}
```

### for

Classic format with initialization, condition, and step:

```nova
for(int i = 0; i < 10; i++) {
    print(i);
}
```

The step can be any assignment:

```nova
for(int i = 0; i < 100; i += 5) {
    print(i);
}

for(int i = 10; i > 0; i--) {
    print(i);
}
```

The loop variable can be declared in the `for` or already exist:

```nova
int i = 0;
for(i = 0; i < 5; i++) {
    print(i);
}
```

---

## 7. Functions

### Declaration

```nova
type functionName(type param1, type param2) {
    // body
    return value;
}
```

### Functions with return values

```nova
int add(int a, int b) {
    return a + b;
}

float area(float base, float height) {
    return base * height / 2.0;
}
```

### Void functions

Functions that return no value use `void`. The `return;` at the end is optional:

```nova
void printValue(int x) {
    print(x);
}

void greet(string name) {
    print("Hello, ");
    print(name);
    return;   // optional
}
```

### Calling functions

```nova
int result = add(3, 4);
printValue(result);
greet("world");
```

Functions can be called as statements (ignoring the return value):

```nova
add(10, 20);       // return value discarded
printValue(42);
```

### Recursion

```nova
int factorial(int n) {
    if(n <= 1) then {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### Declaration Order

Functions must be declared **before** they are called. Declare helper functions at the top of the file.

```nova
// correct
int double(int x) {
    return x * 2;
}

int main() {
    int r = double(5);
    return 0;
}
```

---

## 8. Arrays

### Declaration

Arrays have a fixed size defined at declaration time. The size must be an integer literal.

```nova
int nums[10];          // array of 10 integers, zeroed
float values[5];
```

### Initialization

```nova
int primes[5] = {2, 3, 5, 7, 11};
float coords[3] = {1.0, 2.5, 3.0};
```

### Access and Assignment

```nova
int x = primes[0];     // read → x = 2
primes[2] = 99;        // write
```

Indices can be expressions:

```nova
int i = 2;
int val = primes[i];
primes[i + 1] = 42;
```

### Global Arrays

Arrays can be declared at global scope:

```nova
int table[1000];

int main() {
    table[0] = 1;
    table[999] = 42;
    return 0;
}
```

> **Warning:** Nova does not perform bounds checking at runtime. Accessing beyond the declared size causes undefined behavior.

---

## 9. Print

`print` is a built-in statement that prints a value followed by a newline. It accepts `int`, `float`, or `string`:

```nova
print(42);
print(3.14);
print("Hello!");

int x = 10;
print(x);
```

Expressions can be printed directly:

```nova
print(x + y);
print(x * 2);
```

> `print` accepts only one argument at a time. To print multiple values, use multiple `print` calls.

---

## 10. Comments

Only single-line comments are supported, using `//`:

```nova
// This is a comment
int x = 10;   // comment at end of line
```

---

## 11. Compiling with Nova

### Basic usage

```bash
n++ file.npp
```

Generates an executable with the same name as the source file (without extension).

### Specify output with `-o`

```bash
n++ file.npp -o my_program
n++ file.npp -o output.exe
n++ file.npp -o app.out
```

### Optimizations

```bash
n++ file.npp -O2   # balanced optimizations
n++ file.npp -O3   # aggressive optimizations (faster binary, slower compile)
```

Combined:

```bash
n++ file.npp -o release.out -O3
```

### Version

```bash
n++ --version
# Nova Compiler - Beta 1.0.0
```

### Supported Output Extensions

| Extension        | Result                        |
|------------------|-------------------------------|
| *(none)*         | Native executable             |
| `.exe`           | Executable                    |
| `.out`           | Executable                    |
| `.so`            | Shared library                |
| `.a`             | Static library                |

---

## 12. Full Examples

### Fibonacci

```nova
int fib(int n) {
    if(n <= 1) then {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    for(int i = 0; i <= 10; i++) {
        print(fib(i));
    }
    return 0;
}
```

### Sieve of Eratosthenes

```nova
int limit = 100000;
int isPrime[100001];

int main() {
    // initialize all as prime (0 = prime)
    for(int i = 2; i <= limit; i++) {
        isPrime[i] = 0;
    }

    // mark composites
    for(int i = 2; i <= limit; i++) {
        if(isPrime[i] == 0) then {
            int j = i * 2;
            while(j <= limit) {
                isPrime[j] = 1;
                j += i;
            }
        }
    }

    // print primes
    for(int n = 2; n <= limit; n++) {
        if(isPrime[n] == 0) then {
            print(n);
        }
    }

    return 0;
}
```

### Iterative Factorial

```nova
int factorial(int n) {
    int result = 1;
    for(int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int main() {
    print(factorial(10));
    return 0;
}
```

### FizzBuzz

```nova
int main() {
    for(int i = 1; i <= 100; i++) {
        if(i == 15) then {
            print("FizzBuzz");
        } else if(i == 3) then {
            print("Fizz");
        } else if(i == 5) then {
            print("Buzz");
        } else {
            print(i);
        }
    }
    return 0;
}
```

### Sum of Array

```nova
int sum(int arr[10], int size) {
    int total = 0;
    for(int i = 0; i < size; i++) {
        total += arr[i];
    }
    return total;
}

int main() {
    int numbers[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    print(sum(numbers, 10));
    return 0;
}
```

---

## Quick Reference

```nova
// Types
int  float  string  void

// Declaration
int x = 10;
float y = 3.14;
string s = "text";

// Assignment
x = 5;  x += 1;  x -= 1;  x *= 2;  x /= 2;  x++;  x--;

// Condition
if(cond) then { } else if(cond2) then { } else { }

// Loops
while(cond) { }
for(int i = 0; i < n; i++) { }

// Function
int add(int a, int b) { return a + b; }
void print_val(int x) { print(x); }

// Array
int arr[10];
int arr[3] = {1, 2, 3};
arr[0] = 42;
int v = arr[0];

// Print
print(value);

// Logical operators
&&  ||  !

// Comment
// single line
```