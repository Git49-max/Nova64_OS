# Stellar Programming Language

> A deterministic, C-like language compiled to a compact virtual machine.

Stellar is an imperative programming language designed for simplicity, predictability, and explicit control.
Its syntax is inspired by C, while its runtime behavior is deterministic and minimal.

Stellar emphasizes:

* Explicit numeric types
* Structured control flow
* Manual memory semantics
* No hidden allocations
* No garbage collection

---


# Table of Contents

* Introduction
* Language Overview
* Types
* Variables and Arrays
* Operators
* Control Flow
* Input and Output
* Formal Grammar (EBNF)
* Undefined Behavior
* Complete Example

---


# Introduction

Stellar is a statically structured, imperative language with:

* Two primitive numeric types (`int` and `let`)
* Dynamic arrays
* Structured `if` and `while`
* Explicit input and output

Programs are compiled and executed deterministically.

---


# Language Overview

## Comments

```stellar
// This is a comment
```

## Statements

All statements must end with a semicolon:

```stellar
let x = 5.0;
int y = 10;
```

Missing semicolons produce:

```
error: Expected ';' after previous instruction
```

---

# Types

## `int`

Signed integer type.

```stellar
int i = 10;
```

## `let`

Double-precision floating-point type.

```stellar
let x = 3.14;
```

---

# Variables and Arrays

## Variable Declaration

```stellar
int counter = 0;
let value = 5.0;
```

## Arrays

```stellar
int primes = [1000];
```

Access:

```stellar
primes[5] = 1;
int x = primes[i];
```

⚠ No bounds checking is performed.

---


# Operators

## Arithmetic

```
+  -  *  /  %
```

## Comparison

```
==  !=  <  >  <=  >=
```

Comparisons evaluate to:

* `1` (true)
* `0` (false)

## Assignment

```
=
+=
-=
*=
```

`%=` is not supported.

## Increment / Decrement

```stellar
i++;
i--;
```

---


# Control Flow

## If

```stellar
if (x == 0) {
    print("Zero");
} else {
    print("Non-zero");
}
```

## While

```stellar
while (i < 10) {
    i++;
}
```

---


# Input and Output

## Print

```stellar
print("Hello");
print(value);
```

## Input

```stellar
let x = 0.0;
input(x);
```

Reads a floating-point number.

---

# Formal Grammar (EBNF)

```
program        = { statement } ;

statement      =
      declaration ";"
    | assignment ";"
    | increment ";"
    | if_statement
    | while_statement
    | print_statement ";"
    | input_statement ";"
    ;

declaration    =
      type identifier "=" expression
    | type identifier "=" "[" expression "]"
    ;

type           = "int" | "let" ;

assignment     =
      identifier "=" expression
    | identifier "+=" expression
    | identifier "-=" expression
    | identifier "*=" expression
    | identifier "[" expression "]" "=" expression
    ;

increment      = identifier "++" | identifier "--" ;

if_statement   =
      "if" "(" expression ")" block
      [ "else" block ]
    ;

while_statement =
      "while" "(" expression ")" block ;

block          = "{" { statement } "}" ;

print_statement =
      "print" "(" expression_or_string ")" ;

input_statement =
      "input" "(" identifier ")" ;

expression     = equality ;

equality       = comparison
               | equality "==" comparison
               | equality "!=" comparison ;

comparison     = term
               | comparison "<" term
               | comparison ">" term
               | comparison "<=" term
               | comparison ">=" term ;

term           = factor
               | term "+" factor
               | term "-" factor ;

factor         = unary
               | factor "*" unary
               | factor "/" unary
               | factor "%" unary ;

unary          = primary ;

primary        =
      number
    | identifier
    | identifier "[" expression "]"
    | "(" expression ")"
    ;
```

---

# Undefined Behavior

The following situations produce undefined behavior:

* Stack overflow
* Stack underflow
* Division by zero
* Invalid array index
* Invalid pointer access
* Exceeding variable limits
* Using uninitialized variables

No runtime safety checks are guaranteed.

---

# Complete Example

## Prime Sieve + Simple Cryptography Engine

```stellar
// Prime sieve initialization
int is_prime = [100001];
int i = 2;

while (i < 100001) {
    int status = is_prime[i];
    if (status == 0) {
        int j = i * 2;
        while (j < 100001) {
            is_prime[j] = 1;
            j += i;
        }
    }
    i++;
}

print("--- Stellar Cryptography Engine ---");
print("Choose a number (0-100000):");

let user_input = 0.0;
input(user_input);

print("Enter your key:");
let key = 0.0;
input(key);

let m = 100001.0;
let seed = user_input;

seed *= 31415.0;
seed += 27182.0;
seed = seed % m;

int index = seed;
int prime_flag = is_prime[index];

let result = 0.0;

if (prime_flag == 0) {
    print("Log: Prime Path Detected");
    result = (m * seed + key) % 27182.0;
} else {
    print("Log: Composite Path Detected");
    result = (seed * 27182.0 + key) % m;
}

print("Final Encrypted Result:");
print(result);
```

---

# Summary

Stellar is designed for:

* Explicit numeric computation
* Deterministic structured programming
* Minimal runtime abstraction
* Educational and experimental systems

It intentionally avoids complexity in favor of clarity and predictability.

