# 🚀 Stellar Language

![Stellar Version](https://img.shields.io/badge/version-0.1.0--alpha-blueviolet?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Nova64__OS%20%7C%20Linux-orange?style=for-the-badge)
![Pure C](https://img.shields.io/badge/made%20with-Pure%20C-00599C?style=for-the-badge&logo=c)

**Stellar** is a minimalist, high-performance programming language powered by a Stack-based Virtual Machine. Originally designed to run as a native tool for the **Nova64_OS** ecosystem.

## ✨ Key Features

* **Stack-Based Architecture:** Efficient execution using custom bytecode.
* **64-bit Floating Point:** Native support for `double` precision arithmetic.
* **Control Flow:** Robust `while` loops and `if/else` conditional branching.
* **Operator Precedence:** Complex mathematical expressions with support for parentheses `()`.
* **Smart Diagnostics:** "Did you mean?" suggestion engine powered by the **Levenshtein Distance** algorithm.
* **Deterministic Memory:** Fixed variable slots and stack management (No GC pauses).

---

## 🛠 Project Structure

| File | Role |
| :--- | :--- |
| `stellar.c` | **The Core**: Virtual Machine that interprets the bytecode. |
| `stellar_compiler.c` | **The Brain**: Translates source code into binary instructions. |
| `stellar_errors.c` | **The Helper**: Manages diagnostic messages and fuzzy string matching. |
| `host_main.c` | **The Bridge**: CLI entry point for Linux/Host environments. |

---

## 💻 Code Example

Stellar handles complex logic, such as calculating Factorials with ease:

```stellar
let limit = 10;
let result = 1;
let i = 1;

while (i <= limit) {
    result = result * i;
    print(result);
    i = i + 1;
}

// Complex precedence test
let check = (result / limit) + (100 * 2) % 3;
print(check);

```

---

## 🚀 Getting Started

### 1. Build the Compiler

Using GCC on Linux or within the Nova64 build environment:

```bash
make stellar

```
**Note**: The official repository already have compilated the last version of Stellar. You only need to compile if you change something.
### 2. Compile a Script

```bash
./stellar script.ste

```

This generates a `script.exe` containing the Stellar bytecode, and automatically calls the VM, runs your code, and if you did not kill the program with ^C, it will automatically delete the binary file. 


---

## 🧠 Smart Error Handling

Stellar doesn't just fail; it guides you. If you mistype a function name like `pirnt(x)`, the compiler calculates the edit distance between your input and valid keywords:

```text
####.ste:5:1: error: Unknown function call. Did you mean "print"?
5 | pirnt(10);
  | ^^^^^

```

---

### 📜 Syntax & Grammar

Stellar uses a **delimited grammar** where the semicolon `;` acts as a **Statement Terminator**.

* **Stack Synchronization**: In assignments (e.g., `let x = 10;`) and function calls, the `;` triggers the `OP_STORE` instruction, ensuring the stack is cleared and the state is persisted.
* **Control Flow**: Blocks delimited by braces `{ }` do not require a trailing semicolon.

#### Variables & Memory
Currently, Stellar uses a **Static Memory Mapping** system:
- **Capacity**: Maximum of 26 concurrent variables (`a` through `z`).
- **Identifier Logic**: The compiler performs a simple mapping based on the first character of the identifier. 
  > *Note: `apple` and `age` will currently point to the same memory slot. Full Symbol Table support is coming in the next update!*

## 🗺️ Roadmap

* [x] Math Expressions & Precedences
* [x] Loops & Conditionals
* [x] Smart "Did you mean" Suggestions
* [ ] **Next:** Symbol Table (Full-length variable names)
* [ ] String Literal Support
* [ ] User-defined Functions

---

**Developed by Saulo | Powered by Nova64_OS**
