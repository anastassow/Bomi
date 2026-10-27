# LLVM IR: what CC needs (functions, binary and bitwise operations)

This file explains the part of LLVM IR that CC needs right now:

- writing **functions** (`define`, parameters, `ret`, `call`, `declare`),
- **binary arithmetic** (`+ - * / %`),
- **bitwise operations** (`& | ^ << >>`, and `~` / unary `-`),
- **comparisons** (`< > <= >= == !=`),
- and only as much about **types** and the **module** as you need to write those instructions.

Everything else (variables in memory, `if`/`while`, pointers, structs) is left out on purpose.
The big reference is [IR.md](IR.md) and the official one is the [LLVM Language Reference](https://llvm.org/docs/LangRef.html).

Every example in this file was checked with LLVM 23.1.4 (`opt -passes=verify`) and run with `lli`.
The comment next to an instruction shows the value it produces.

---

## Contents

1. [The smallest valid program](#1-the-smallest-valid-program)
2. [How to check your output](#2-how-to-check-your-output)
3. [Values and names](#3-values-and-names)
4. [Integer types](#4-integer-types)
5. [The module (the `.ll` file)](#5-the-module-the-ll-file)
6. [Functions](#6-functions)
7. [Binary arithmetic: add, sub, mul, div, rem](#7-binary-arithmetic-add-sub-mul-div-rem)
8. [Bitwise operations: and, or, xor, shifts](#8-bitwise-operations-and-or-xor-shifts)
9. [Unary operators built from binary ones](#9-unary-operators-built-from-binary-ones)
10. [Comparisons: icmp](#10-comparisons-icmp)
11. [Changing the width: zext, sext, trunc](#11-changing-the-width-zext-sext-trunc)
12. [Poison and undefined behavior](#12-poison-and-undefined-behavior)
13. [From CC's AST to IR](#13-from-ccs-ast-to-ir)
14. [Error messages and what they mean](#14-error-messages-and-what-they-mean)
15. [Cheat sheet](#15-cheat-sheet)

---

## 1. The smallest valid program

```llvm
define i32 @main() {
entry:
  ret i32 42
}
```

Run it:

```sh
$ /opt/homebrew/opt/llvm/bin/lli min.ll ; echo $?
42
```

Line by line:

| Text | Meaning |
|---|---|
| `define` | "here is a function with a body" |
| `i32` | the function returns a 32-bit integer (a C `int`) |
| `@main` | the function's name. `@` means a global name (functions are global) |
| `()` | no parameters |
| `{ ... }` | the body |
| `entry:` | a **label**. It names the first **basic block** of the function |
| `ret i32 42` | return the value `42`, of type `i32` |

That is a complete, valid LLVM IR file. Everything below adds to this shape.

---

## 2. How to check your output

LLVM is installed at `/opt/homebrew/opt/llvm/bin`. Make a shortcut:

```sh
LLVM=/opt/homebrew/opt/llvm/bin
```

| Command | What it does |
|---|---|
| `$LLVM/opt -passes=verify -disable-output out.ll` | **Checks** that the file is valid LLVM IR. Prints nothing if it is fine. |
| `$LLVM/lli out.ll ; echo $?` | **Runs** the file directly and shows what `main` returned |
| `$LLVM/clang out.ll -o out && ./out` | Compiles it to a real program |
| `$LLVM/opt -passes=instcombine -S out.ll` | Runs one optimization and prints the result (`-S` = print as text) |
| `$LLVM/clang -S -emit-llvm -O0 -Xclang -disable-O0-optnone t.c -o -` | Shows what **clang** emits for a C file. Use it to compare when you are unsure. |

**Exit codes only keep the low 8 bits.** `ret i32 1000` shows as `232` (1000 mod 256), and `-1` shows as `255`.
For bigger or negative results, print with `printf` ([§6.6](#66-calling-printf-to-see-results)).

`clang out.ll` may print `warning: overriding the module target triple`. That is harmless ([§5](#5-the-module-the-ll-file)).

---

## 3. Values and names

### 3.1 Every instruction makes a new value

```llvm
  %sum = add i32 2, 3          ; %sum = 5
```

Read it as: *"compute `add` of the `i32` values `2` and `3`, and call the result `%sum`."*

- `%sum` is a **local name** (`%` = local to this function).
- `@main`, `@printf` are **global names** (`@` = visible in the whole file).
- `2`, `3` are **constants**. They can be used directly as operands. There is no "load immediate" instruction.

### 3.2 SSA: each name is assigned exactly once

LLVM IR is in **SSA form** (Static Single Assignment). A name gets its value **once** and never changes:

```llvm
  %x = add i32 2, 3
  %x = add i32 %x, 1           ; ERROR: multiple definition of local value named 'x'
```

So you never "update" a value. You make a new one:

```llvm
  %x  = add i32 2, 3           ; 5
  %x1 = add i32 %x, 1          ; 6
```

This is why a compiler usually uses a counter for names: `%t0`, `%t1`, `%t2`, ...
A value must also be **defined before it is used** (in the order the instructions run).

### 3.3 Named vs numbered values

There are two kinds of local names:

| Kind | Example | Rule |
|---|---|---|
| named | `%t0`, `%sum`, `%a.b_c` | any letters, digits, `.`, `_`, `$`. Must not start with a digit. |
| numbered | `%0`, `%1`, `%2` | LLVM checks the numbers, and the rules are easy to break |

The trap with numbered values: an unnamed parameter or an **entry block without a label** also takes a number.

```llvm
define i32 @main() {           ; no "entry:" label → the block silently becomes %0
  %0 = add i32 1, 2            ; ERROR: instruction expected to be numbered '%1' or greater
  ret i32 %0
}
```

**Recommendation for CC:** always write the `entry:` label and always use **named** values like `%t0`.
Then the numbering rules never matter. (This is what CC's `ir.c` already does with `%vN`.)

---

## 4. Integer types

### 4.1 Why types are needed

In LLVM IR **every value has a type, and every instruction writes it**. There are no untyped numbers:

```llvm
  %x = add 2, 3                ; ERROR: expected type
  %x = add i32 2, 3            ; OK
```

When CC turns a number from the AST into IR, it must decide its type.

### 4.2 `iN`: an integer of N bits

| Type | Bits | Used for in C | Range if read as signed |
|---|---|---|---|
| `i1` | 1 | result of a comparison (true/false) | `true` = 1, `false` = 0 |
| `i8` | 8 | `char` | −128 … 127 |
| `i16` | 16 | `short` | −32768 … 32767 |
| `i32` | 32 | **`int`** | −2147483648 … 2147483647 |
| `i64` | 64 | `long`, `long long` (on your Mac) | about ±9.2·10¹⁸ |

Any width works (`i3`, `i33`, `i128`), but C only needs these.

### 4.3 Integers have no sign

This is the most important idea in this section. **An `i32` is just 32 bits.** It is not "signed int" or "unsigned int".

The bit pattern `11111111 11111111 11111111 11111111` is one `i32` value. It means −1 if you read it as signed
(two's complement) and 4294967295 if you read it as unsigned. Both constants below are **the same value**:

```llvm
  %a = add i32 -1, 0           ; bits 0xFFFFFFFF
  %b = add i32 4294967295, 0   ; the same bits 0xFFFFFFFF
```

So where does signed vs unsigned go? **Into the instruction.** When the result depends on the sign, there are two instructions:

| Operation | Signed (C `int`) | Unsigned (C `unsigned`) |
|---|---|---|
| divide | `sdiv` | `udiv` |
| remainder | `srem` | `urem` |
| shift right | `ashr` | `lshr` |
| compare `<` | `icmp slt` | `icmp ult` |

When the result is the **same** for both (`add`, `sub`, `mul`, `and`, `or`, `xor`, `shl`, `==`), there is only one instruction.
This works because of two's complement: adding the bits gives the right answer either way.

For CC (which only has `int` today): always pick the **signed** version (`sdiv`, `srem`, `ashr`, `slt`...).

### 4.4 Writing constants

| Constant | Meaning |
|---|---|
| `i32 42`, `i32 -7` | decimal, can be negative |
| `i1 true`, `i1 false` | for `i1` (same as `1` and `0`) |
| `i32 0` | zero |

**Warning: a constant that does not fit is silently cut off.** LLVM does **not** report an error:

```llvm
  %x = add i32 4294967296, 0   ; 2^32 does not fit in 32 bits → becomes 0
  ret i8 300                   ; becomes 44 (300 - 256)
```

CC must make sure the number fits the type it chooses.

### 4.5 Choosing the type for a C number

C decides the type of a literal like `5` or `3000000000`. Without a suffix (`L`, `U`), a decimal literal gets the first type that can hold it:

| Literal | C type | LLVM type |
|---|---|---|
| `5`, `1000000` | `int` | `i32` |
| `3000000000` (too big for `int`) | `long` | `i64` |

Your AST stores numbers as `long value`. So: if the value is between −2147483648 and 2147483647 use `i32`, otherwise `i64`.

### 4.6 Both operands must have the same type

```llvm
  %x = add i32 2, 3
  %y = add i64 %x, 1           ; ERROR: '%x' defined with type 'i32' but expected 'i64'
```

The type is written **once** and applies to both operands and the result. If C mixes `int` and `long`,
first widen the `i32` to `i64` ([§11](#11-changing-the-width-zext-sext-trunc)).

---

## 5. The module (the `.ll` file)

A **module** is what LLVM calls one `.ll` file. It is a list of top-level things, in any order:

```llvm
; ---- optional header: which machine this is for ----
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx26.0.0"

; ---- global variables / constant data (only needed for printf's format string) ----
@.fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00"

; ---- functions from outside the file ----
declare i32 @printf(ptr, ...)

; ---- your functions ----
define i32 @test() {
entry:
  ret i32 0
}

define i32 @main() {
entry:
  ret i32 0
}
```

What you need to know:

- **One `define` per C function.** Your `input.txt` has `test` and `main`, so the output has two `define`s.
- **Functions can be in any order.** `main` can call `@test` even if `@test` is defined below it.
- **Comments** start with `;` and go to the end of the line.
- **`target triple` and `target datalayout`** describe your machine (CPU, pointer size...). Both are **optional**.
  Without them the file still verifies and runs. Copy them from `clang -S -emit-llvm` output when you want
  the exact same setup as clang. The two lines above are what clang 23 emits on your Mac.
- **Names must be unique** in the module: two `define i32 @main` is an error.

---

## 6. Functions

### 6.1 The syntax

```
define <return type> @<name>(<type> %<param>, <type> %<param>, ...) {
<label>:
  <instructions>
  <one terminator, e.g. ret>
}
```

Example: a C function `int calc(int a, int b) { return (a * 3) / (b - 1) & 255; }`

```llvm
define i32 @calc(i32 %a, i32 %b) {
entry:
  %t0 = mul nsw i32 %a, 3
  %t1 = sub nsw i32 %b, 1
  %t2 = sdiv i32 %t0, %t1
  %t3 = and i32 %t2, 255
  ret i32 %t3
}
```

- **Parameters** are values like any other: `%a` and `%b` can be used directly. They are already defined when the function starts.
- The **return type** is written before the name, and `ret` must return a value of exactly that type.
- A function **without a return value** returns `void`, and ends with `ret void`:

```llvm
define void @nothing() {
entry:
  ret void
}
```

### 6.2 Basic blocks and terminators

The body is made of **basic blocks**. A basic block is:

1. an optional label (`entry:`),
2. a list of normal instructions,
3. exactly **one terminator** at the end: an instruction that leaves the block.

For now the only terminator you need is **`ret`**. (Later, `br` jumps to another block for `if`/`while`.)

Rules that cause errors:

| Mistake | What happens |
|---|---|
| block without `ret` at the end | `error: expected instruction opcode` (pointing at the `}`) |
| instructions **after** `ret` | the same error. LLVM thinks they start a new block that never ends |
| two `ret` in a row | the same problem |

So for C code like `return 1; 2 + 3;`, the code after `return` can never run. Simply **do not emit it**
(the full IR.md shows the other option: put it in a new, unreachable block).

**A function that falls off the end** (no `return` in C) still needs a terminator. Do what clang does:

- for `main`: `ret i32 0` (C says `main` returns 0 when it reaches the end),
- for other `int` functions: any value, e.g. `ret i32 0`. (C says using that result is undefined. LLVM's own
  choice would be `ret i32 poison`, see [§12](#12-poison-and-undefined-behavior).)

### 6.3 Calling a function

```llvm
define i32 @square(i32 %x) {
entry:
  %r = mul nsw i32 %x, %x
  ret i32 %r
}

define i32 @sum3(i32 %a, i32 %b, i32 %c) {
entry:
  %ab  = add nsw i32 %a, %b
  %abc = add nsw i32 %ab, %c
  ret i32 %abc
}

define i32 @main() {
entry:
  %s = call i32 @square(i32 5)                      ; 25
  %r = call i32 @sum3(i32 %s, i32 10, i32 -3)       ; 25 + 10 - 3 = 32
  ret i32 %r                                        ; exit code 32
}
```

`call` syntax:

```
%result = call <return type> @<name>(<type> <value>, <type> <value>, ...)
```

- Every argument has its type written in front of it.
- The return type is written after `call`.
- A call to a `void` function has **no** `%result =`: `call void @nothing()`.
- The result of a call is a normal value: you can use `%s` in the next instruction.

**Warning: LLVM does not check that the arguments match.** This passes `opt -passes=verify` with no error:

```llvm
define i32 @f(i32 %a) {
entry:
  ret i32 %a
}
define i32 @main() {
entry:
  %r = call i32 @f(i64 1)      ; f wants an i32 but gets an i64: no error, but undefined behavior
  ret i32 %r
}
```

So CC itself must check that the call uses the same types as the `define`.

### 6.4 `declare`: functions from outside

To call a function that is **not in your file** (like `printf` from the C library), you write its signature with `declare`:

```llvm
declare i32 @printf(ptr, ...)
```

- `declare` = signature only, no body, no parameter names needed.
- `ptr` is the type of a pointer (here: the format string).
- `...` means "variable number of arguments" (varargs), like in C.

Calling a varargs function needs the **whole function type** in the call, `i32 (ptr, ...)`:

```llvm
  %n = call i32 (ptr, ...) @printf(ptr @.fmt, i32 %t0)
```

For a normal function, `call i32 @f(...)` is enough. For a varargs one it must be `call i32 (ptr, ...) @printf(...)`.

Calling a function that is neither defined nor declared gives `error: use of undefined value '@foo'`.

### 6.5 Who can see a function: `internal`

By default a function is visible to other files (the linker can find it). `main` must stay that way.
A helper used only in this file can be marked `internal`, which tells the optimizer that nobody else calls it:

```llvm
define internal i32 @helper(i32 %x) {
entry:
  ret i32 %x
}
```

This is optional, but it matters for optimizations: an `internal` function that is inlined everywhere can be deleted.
C's `static int helper(...)` becomes `internal`.

### 6.6 Calling `printf` to see results

A complete program that prints `42`:

```llvm
@.fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %t0 = mul nsw i32 6, 7
  %t1 = call i32 (ptr, ...) @printf(ptr @.fmt, i32 %t0)
  ret i32 0
}
```

The first line makes a global constant holding the C string `"%d\n"`:

| Part | Meaning |
|---|---|
| `@.fmt` | its name (a global, so `@`) |
| `private` | only visible in this file |
| `unnamed_addr` | its address is not important (LLVM may merge equal strings) |
| `constant` | read-only |
| `[4 x i8]` | an array of 4 bytes: `%`, `d`, newline, zero byte |
| `c"%d\0A\00"` | the bytes. `\0A` = newline (hex 0A), `\00` = the terminating zero |

Using `@.fmt` as an argument passes its address, which is what `printf` expects (`ptr`).

### 6.7 What clang adds (and you can leave out)

Clang's output has much more on a function:

```llvm
define dso_local i32 @square(i32 noundef %x) #0 {
```

All of this is **optional**. Your IR is valid without it:

| Extra | Meaning | Do you need it? |
|---|---|---|
| `dso_local` | "this function is in the same program" (helps code generation) | no |
| `noundef` on a parameter | "this argument is never undef/poison" (helps optimization) | no, but useful later |
| `#0` | a group of function attributes listed at the end of the file (`nounwind`, `noinline`...) | no |
| `!llvm...` lines | metadata (compiler version...) | no |

Leave them out at first. Add `noundef` later if you want your IR to optimize exactly like clang's.

---

## 7. Binary arithmetic: add, sub, mul, div, rem

All of these have the same form:

```
%result = <opcode> [flags] <type> <left>, <right>
```

Both operands and the result have the same type.

### 7.1 `add`, `sub`, `mul`

```llvm
  %a = add i32 7, 3            ; 10
  %b = sub i32 7, 3            ; 4
  %c = mul i32 7, 3            ; 21
```

**They wrap around on overflow.** The result is always computed modulo 2³² for `i32` (only the low 32 bits are kept):

```llvm
  %f = mul i32 1000000, 1000000      ; -727379968 (10^12 does not fit; low 32 bits kept)
```

That is exactly the line `1000000 * 1000000;` in `input.txt`. In C, signed overflow is **undefined behavior**.
In LLVM a plain `mul` just wraps. To tell LLVM about C's rule, you add a flag.

### 7.2 The flags `nsw` and `nuw`

| Flag | Name | Promise to LLVM |
|---|---|---|
| `nsw` | **n**o **s**igned **w**rap | "read as signed, this never overflows" |
| `nuw` | **n**o **u**nsigned **w**rap | "read as unsigned, this never overflows" |

```llvm
  %x = add nsw i32 %a, %b
  %y = add nuw nsw i32 %a, %b        ; both flags (order: nuw nsw)
```

If the promise is broken, the result is **poison** ([§12](#12-poison-and-undefined-behavior)), a value that means "this computation went wrong".
This matches C: signed overflow is undefined.

**Why bother? Because it lets the optimizer do more.** Here is the same C function `return (a + 1) > a;`
compiled two ways, after `opt -passes=instcombine`:

```llvm
; without nsw: a + 1 can wrap (2147483647 + 1 = -2147483648), so the compare must stay
  %b = add i32 %a, 1
  %c = icmp sgt i32 %b, %a
; → optimized to:
  %c = icmp ne i32 %a, 2147483647

; with nsw: no wrap is possible, so (a + 1) > a is always true
  %b = add nsw i32 %a, 1
  %c = icmp sgt i32 %b, %a
; → optimized to:
  ret i32 1
```

**For CC:** use `add nsw`, `sub nsw`, `mul nsw` for C `int` (that is what clang does).
For C `unsigned`, use **no** flag (unsigned overflow is defined in C, it wraps).

### 7.3 Division: `sdiv` and `udiv`

```llvm
  %a = sdiv i32 7, 2           ; 3
  %b = sdiv i32 -7, 2          ; -3   (rounds toward zero, like C)
  %c = udiv i32 -7, 2          ; 2147483644   (-7 read as unsigned is 4294967289)
```

- `sdiv` = **s**igned divide, rounds toward zero (C99 and later say the same for `/`).
- `udiv` = **u**nsigned divide.

**Division by zero is undefined behavior** (worse than poison: the program is broken the moment it runs, and on some machines it crashes).
The same for `sdiv` of the smallest number by −1 (`-2147483648 / -1`), because the answer 2147483648 does not fit in `i32`.

The IR is still **valid**: `sdiv i32 7, 0` passes the verifier. It is only wrong at runtime.
The optimizer may use it: `opt -passes=instcombine` turns `ret (sdiv i32 7, 0)` into `ret i32 poison`.

**Signed and unsigned give different code.** Dividing by 2:

```llvm
  %q = udiv i32 %a, 2          ; optimized to:  lshr i32 %a, 1   (just a shift)
  %q = sdiv i32 %a, 2          ; stays sdiv     (a shift rounds -7/2 to -4, but C wants -3)
```

Optional flag: `sdiv exact` / `udiv exact` promises the division has no remainder (otherwise poison). You don't need it yet.

### 7.4 Remainder: `srem` and `urem`

```llvm
  %a = srem i32 7, 2           ; 1
  %b = srem i32 -7, 2          ; -1   (sign follows the left operand, like C's %)
  %c = urem i32 -7, 2          ; 1    (4294967289 mod 2)
```

- **There is no `mod` instruction.** `mod i32 7, 2` gives `error: expected instruction opcode`.
  C's `%` on `int` is **`srem`**.
- Remainder by zero is undefined behavior, same as division.

`srem` is not the mathematical modulo: `-7 mod 2` would be `1` in math, but C and `srem` give `-1`.

---

## 8. Bitwise operations: and, or, xor, shifts

These work on the bits. Signedness does not matter for `and`/`or`/`xor`/`shl`.

### 8.1 `and`, `or`, `xor`

```llvm
  %a = and i32 6, 3            ; 2     0110 & 0011 = 0010
  %b = or  i32 12, 3           ; 15    1100 | 0011 = 1111
  %c = xor i32 6, 1            ; 7     0110 ^ 0001 = 0111
```

| C | IR |
|---|---|
| `a & b` | `and i32 %a, %b` |
| `a \| b` | `or i32 %a, %b` |
| `a ^ b` | `xor i32 %a, %b` |

These never overflow and never produce poison by themselves.

Optional flag: `or disjoint` promises that the two operands have **no 1-bit in common** (then `or` is the same as `add`).
If they share a bit, the result is poison. Clang never needs this for plain C `|`. Optimizations add it.

### 8.2 Shifts: `shl`, `lshr`, `ashr`

```llvm
  %a = shl  i32 1, 4           ; 16           shift left, fill with 0
  %b = lshr i32 -8, 1          ; 2147483644   logical shift right, fill with 0
  %c = ashr i32 -8, 1          ; -4           arithmetic shift right, copy the sign bit
```

| C | `int` (signed) | `unsigned` |
|---|---|---|
| `a << n` | `shl` | `shl` |
| `a >> n` | **`ashr`** | `lshr` |

Why two right shifts? `-8` is `1111...1000`.
`lshr` fills from the left with `0` → `0111...1100` (a big positive number).
`ashr` fills with copies of the top bit → `1111...1100` = −4, which is what `-8 >> 1` means for a signed number.

**Shifting by the width or more gives poison:**

```llvm
  %x = shl i32 1, 32           ; poison (i32 has only 32 bits, so the amount must be 0..31)
  %y = shl i32 1, 31           ; -2147483648 (OK: the top bit is set)
```

This matches C, where shifting by the width or more (or by a negative amount) is undefined.

**The shift amount has the same type as the value.** In C, `x << n` with `x` an `int` and `n` a `long` is fine.
In IR both must be `i32` (convert `n` first with `trunc`, see [§11](#11-changing-the-width-zext-sext-trunc)).
The result has the **type of the left operand** in C too.

Optional flags: `shl nuw`/`shl nsw` (poison if bits that matter are shifted out), and `lshr exact`/`ashr exact` (poison if a 1-bit is shifted out).
Clang does not add them for plain C shifts.

---

## 9. Unary operators built from binary ones

LLVM has **no** `neg` or `not` instruction. You build them from binary ones:

| C | IR | Why it works |
|---|---|---|
| `-x` | `%r = sub nsw i32 0, %x` | 0 − x. (`nsw`: `-(-2147483648)` overflows, which is undefined in C) |
| `~x` | `%r = xor i32 %x, -1` | `-1` is all 1-bits, and `xor` with 1 flips a bit |
| `!x` | `%c = icmp eq i32 %x, 0` then `%r = zext i1 %c to i32` | "x is zero" gives true (1) |
| `+x` | nothing: use `%x` itself | |

```llvm
  %g = xor i32 5, -1           ; -6   (~5)
  %n = sub nsw i32 0, 5        ; -5   (-5)
```

Clang emits exactly these, so the optimizer recognizes them.

---

## 10. Comparisons: `icmp`

```
%result = icmp <condition> <type> <left>, <right>
```

The result is always **`i1`** (`true` or `false`), whatever the operand type is.

| Condition | Meaning | C (`int`) |
|---|---|---|
| `eq` | equal | `==` |
| `ne` | not equal | `!=` |
| `slt` | signed less than | `<` |
| `sle` | signed less or equal | `<=` |
| `sgt` | signed greater than | `>` |
| `sge` | signed greater or equal | `>=` |
| `ult` `ule` `ugt` `uge` | the same, unsigned | for `unsigned` |

Signedness matters again:

```llvm
  %h = icmp slt i32 -1, 1      ; true    (-1 < 1)
  %i = icmp ult i32 -1, 1      ; false   (4294967295 < 1 is false)
```

### 10.1 C comparisons give an `int`

In C, `3 < 4` is an **`int`** with value 0 or 1, and you can compute with it: `(3 < 4) + 10` is `11`.
In LLVM, `icmp` gives an `i1`, and you cannot add an `i1` to an `i32`:

```llvm
  %c = icmp slt i32 1, 2
  %r = add i32 %c, 1           ; ERROR: '%c' defined with type 'i1' but expected 'i32'
```

So every C comparison is **two** instructions: `icmp`, then `zext` to widen the `i1` to `i32`:

```llvm
  %c = icmp slt i32 3, 4       ; i1 true
  %r = zext i1 %c to i32       ; i32 1
```

(When the comparison is only used by an `if`, which comes later, the `i1` is used directly and `zext` is not needed.)

### 10.2 What about `&&` and `||`?

In C, `a && b` must **not** evaluate `b` when `a` is false. For example `x != 0 && 10 / x > 1` must not divide by zero.
That needs jumping between basic blocks (`br`) and a `phi` instruction. It is control flow, not a binary instruction,
so it is outside this file. See "Short-circuit `&&` and `||`" in [IR.md](IR.md).

---

## 11. Changing the width: `zext`, `sext`, `trunc`

You need these when two operands have different widths: an `i1` from a comparison, or `int` mixed with `long`.

```
%result = <opcode> <from type> <value> to <to type>
```

| Instruction | Does | Example |
|---|---|---|
| `zext` | **z**ero-**ext**end: make wider, fill with 0 | `zext i1 true to i32` → `1` |
| `sext` | **s**ign-**ext**end: make wider, copy the sign bit | `sext i32 -5 to i64` → `-5` |
| `trunc` | make narrower, drop the high bits | `trunc i64 4294967297 to i32` → `1` |

Which one to use:

- `i1` (comparison) → `i32`: **`zext`** (true must become 1, not −1).
- signed `int` → `long`: **`sext`** (−5 must stay −5). `zext` would turn −5 into 4294967291.
- `unsigned` → wider: **`zext`**.
- wider → narrower (for example a `long` shift amount for an `int` shift): **`trunc`**.

Mixing `int` and `long` in C (`1 + 3000000000`) means: `sext` the `i32` to `i64`, then `add nsw i64`.

---

## 12. Poison and undefined behavior

You will see "poison" everywhere in LLVM. It is how LLVM represents C's "undefined behavior" without crashing at compile time.

### 12.1 Poison

**Poison** is a special value that means "the operation that made me broke a promise".

- It comes from a broken flag (`add nsw` that overflows) or a shift by ≥ the width.
- **It spreads:** `add i32 poison, 1` is poison, `and i32 poison, 0` is poison, ...
- It only becomes a real problem when it decides something: returning it from `main`, dividing by it, using it in an `if`.

Why it exists: it allows LLVM to **assume** the bad case never happens. That is what made `(a + 1) > a` → `1` legal in [§7.2](#72-the-flags-nsw-and-nuw).

### 12.2 Immediate undefined behavior

Some things are worse: they make the whole program meaningless the moment they run.

| Instruction | Poison when | Immediate UB when |
|---|---|---|
| `add`/`sub`/`mul nsw` | signed overflow | — |
| `add`/`sub`/`mul nuw` | unsigned overflow | — |
| `shl`, `lshr`, `ashr` | shift amount ≥ width | — |
| `sdiv`, `srem` | (`exact` and remainder ≠ 0) | divide by **0**, or **INT_MIN / −1** |
| `udiv`, `urem` | (`exact` and remainder ≠ 0) | divide by **0** |
| `and`, `or`, `xor` | (`or disjoint` and common bits) | — |
| `call` | — | argument types do not match the function |

### 12.3 What this means for CC

- Emitting `nsw` is correct **because C already says signed overflow is undefined**. You are only telling LLVM what C says.
- Do **not** put `nsw` on `unsigned` math: unsigned overflow is defined in C (it wraps), so the flag would make correct programs wrong.
- `input.txt` has `1000000 * 1000000;` (signed overflow). With `mul nsw` that value is poison. That is fine, because the C program is undefined there too.

---

## 13. From CC's AST to IR

### 13.1 The idea: each expression node gives back one value

Lowering an expression tree works **bottom-up** (post-order): first lower the children, then the node itself.
Each node produces one thing that can be used as an operand:

- a `NODE_NUM` produces a **constant**, written as text like `6`. No instruction is needed.
- a `NODE_BINOP` lowers its left child and its right child, then emits **one new instruction** that uses them,
  and produces the **new name** (`%t2`).

So the "value" a node gives back has two parts: its **type** (`i32`) and its **text** (`6` or `%t2`).
This is why CC's `Operand` needs a type. Without it, CC cannot write `add i32` vs `add i64`, or know when it needs `sext`.

### 13.2 Worked example

`input.txt` contains `((6 & 1) ^ 10) | 100;`. The tree:

```
        |
       / \
      ^   100
     / \
    &   10
   / \
  6   1
```

Lowering, bottom-up:

| Step | Node | Emits | Gives back |
|---|---|---|---|
| 1 | `6` | nothing | `i32 6` |
| 2 | `1` | nothing | `i32 1` |
| 3 | `&` | `%t1 = and i32 6, 1` | `i32 %t1` |
| 4 | `10` | nothing | `i32 10` |
| 5 | `^` | `%t2 = xor i32 %t1, 10` | `i32 %t2` |
| 6 | `100` | nothing | `i32 100` |
| 7 | `\|` | `%t3 = or i32 %t2, 100` | `i32 %t3` |

A whole program, written the way CC would produce it. `test` returns the expression and `main` calls it:

```c
int test() { 1 + 2; return ((6 & 1) ^ 10) | 100; }
int main() { return 4 * 10 / (40 * (2 - 12)) + test(); }
```

```llvm
define i32 @test() {
entry:
  %t0 = add nsw i32 1, 2           ; expression statement: computed, result not used
  %t1 = and i32 6, 1               ; 0
  %t2 = xor i32 %t1, 10            ; 10
  %t3 = or i32 %t2, 100            ; 110
  ret i32 %t3
}

define i32 @main() {
entry:
  %t0 = mul nsw i32 4, 10          ; 40
  %t1 = sub nsw i32 2, 12          ; -10
  %t2 = mul nsw i32 40, %t1        ; -400
  %t3 = sdiv i32 %t0, %t2          ; 40 / -400 = 0 (rounds toward zero)
  %t4 = call i32 @test()           ; 110
  %t5 = add nsw i32 %t3, %t4       ; 110
  ret i32 %t5                      ; exit code 110
}
```

Notes on this example:

- **Constants are fine as operands.** `and i32 6, 1` is valid IR. Your compiler does not need to calculate anything itself.
  (Clang would fold `6 & 1` to `0` already, and LLVM's optimizer will too. That folding is one of the optimizations you can test.)
- **The counter restarts per function.** `%t0` in `@test` and `%t0` in `@main` do not clash, because `%` names are local to a function.
- An **expression statement** (`1 + 2;`) still emits its instructions. The result is just never used.
  (The optimizer removes such dead code. That is another optimization to test.)

### 13.3 Every C operator in `ast.h`, for `int`

| `Op` | C | IR (`int` = `i32`) |
|---|---|---|
| `OP_ADD` | `a + b` | `add nsw i32 a, b` |
| `OP_SUB` | `a - b` | `sub nsw i32 a, b` |
| `OP_MUL` | `a * b` | `mul nsw i32 a, b` |
| `OP_DIV` | `a / b` | `sdiv i32 a, b` |
| `OP_MOD` | `a % b` | `srem i32 a, b` |
| `OP_SHL` | `a << b` | `shl i32 a, b` |
| `OP_SHR` | `a >> b` | `ashr i32 a, b` |
| `OP_BAND` | `a & b` | `and i32 a, b` |
| `OP_BXOR` | `a ^ b` | `xor i32 a, b` |
| `OP_BOR` | `a \| b` | `or i32 a, b` |
| `OP_LT` | `a < b` | `icmp slt i32 a, b` + `zext i1 … to i32` |
| `OP_GT` | `a > b` | `icmp sgt` + `zext` |
| `OP_LE` | `a <= b` | `icmp sle` + `zext` |
| `OP_GE` | `a >= b` | `icmp sge` + `zext` |
| `OP_EQ` | `a == b` | `icmp eq` + `zext` |
| `OP_NE` | `a != b` | `icmp ne` + `zext` |
| `OP_NEG` | `-a` | `sub nsw i32 0, a` |
| `OP_BNOT` | `~a` | `xor i32 a, -1` |
| `OP_LNOT` | `!a` | `icmp eq i32 a, 0` + `zext` |
| `OP_POS` | `+a` | nothing (use `a`) |
| `OP_COMMA` | `a, b` | lower `a`, ignore its value, give back `b` |
| `OP_LAND`, `OP_LOR` | `a && b`, `a \|\| b` | needs blocks + `phi` (not in this file) |
| assignments, `++`, `--`, `&`, `*` | | need variables in memory (not in this file) |

### 13.4 Things in CC's current code to watch for

- `ir.c` prints everything as `i64`. C `int` is `i32`. The results differ when a value overflows 32 bits (`1000000 * 1000000`).
- `ir.c` has no case for `IR_MOD`. It must print `srem` (there is no `mod`).
- `ir.c` prints one fixed `@main`. With `NODE_FUNC` in the AST, it needs one `define` per function.
- In `parser.y`, the rules for `<< >> < > <= >= == != && ||` have no action. Bison then keeps only the
  **left** operand (`$$ = $1`), so `3 < 4` arrives in the AST as just `3`. Give each rule a `new_binop(...)` like the `+` rule has.
- `return` is not in the AST yet (`jump_statement` gives `NULL`). Until it is, end every function with `ret i32 0`.

---

## 14. Error messages and what they mean

All of these are real messages from `opt -passes=verify` (LLVM 23):

| Error | Cause | Fix |
|---|---|---|
| `expected type` | a type is missing: `add 2, 3` | `add i32 2, 3` |
| `'%x' defined with type 'i32' but expected 'i64'` | operand types differ | use one type. Widen with `sext`/`zext` |
| `'%c' defined with type 'i1' but expected 'i32'` | used a comparison result as an `int` | `zext i1 %c to i32` first |
| `multiple definition of local value named 'x'` | SSA: a name was assigned twice | new name per instruction (`%t0`, `%t1`, ...) |
| `use of undefined value '%x'` | used a name that was never defined (or is in another function) | check your counter, define before use |
| `use of undefined value '@foo'` | called a function that is neither `define`d nor `declare`d | add `declare` (or define it) |
| `value doesn't match function result type 'i32'` | `ret i64 0` in an `i32` function | return the declared type |
| `expected instruction opcode` at `}` | the block has no `ret`, or there are instructions after `ret` | exactly one `ret`, at the end |
| `expected instruction opcode` at a word | unknown instruction, e.g. `mod` | use `srem` / `urem` |
| `instruction expected to be numbered '%1' or greater` | numbered names clash with an unlabeled block/param | use `entry:` and named values |

**Not reported at all** (valid IR, wrong program):

- a constant too big for its type (`i32 4294967296` → `0`),
- calling a function with the wrong argument types,
- division by zero, shift by ≥ width, `nsw` overflow.

---

## 15. Cheat sheet

```llvm
; ---------- module ----------
@.fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00"
declare i32 @printf(ptr, ...)

; ---------- function ----------
define i32 @f(i32 %a, i32 %b) {
entry:
  ; arithmetic (signed int)
  %t0 = add nsw i32 %a, %b         ; a + b
  %t1 = sub nsw i32 %a, %b         ; a - b
  %t2 = mul nsw i32 %a, %b         ; a * b
  %t3 = sdiv i32 %a, %b            ; a / b     (b == 0 → UB)
  %t4 = srem i32 %a, %b            ; a % b     (b == 0 → UB)

  ; bitwise
  %t5 = and i32 %a, %b             ; a & b
  %t6 = or  i32 %a, %b             ; a | b
  %t7 = xor i32 %a, %b             ; a ^ b
  %t8 = shl i32 %a, %b             ; a << b    (b >= 32 → poison)
  %t9 = ashr i32 %a, %b            ; a >> b    (signed; lshr for unsigned)

  ; unary
  %t10 = sub nsw i32 0, %a         ; -a
  %t11 = xor i32 %a, -1            ; ~a
  %t12 = icmp eq i32 %a, 0         ; !a ...
  %t13 = zext i1 %t12 to i32       ; ... as int

  ; comparison as an int
  %t14 = icmp slt i32 %a, %b       ; a < b  (i1)
  %t15 = zext i1 %t14 to i32       ; 0 or 1 (i32)

  ; width changes
  %t16 = sext i32 %a to i64        ; int → long
  %t17 = trunc i64 %t16 to i32     ; long → int

  ; calls
  %t18 = call i32 @g(i32 %t0)
  %t19 = call i32 (ptr, ...) @printf(ptr @.fmt, i32 %t18)

  ret i32 %t15
}

define i32 @g(i32 %x) {
entry:
  ret i32 %x
}

define i32 @main() {
entry:
  %r = call i32 @f(i32 7, i32 2)
  ret i32 0
}
```

Check it: `$LLVM/opt -passes=verify -disable-output cheat.ll` (silent = valid).
