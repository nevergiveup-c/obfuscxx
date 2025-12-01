# obfuscxx

<p align="center">
  <img src="images/banner.png">
</p>

## Description
Header-only compile-time variables obfuscation library for C++20 and later.

## How it works
During compilation, data is encrypted via eXtended Tiny Encryption Algorithm (XTEA). Decryption uses SIMD instructions (AVX/SSE/NEON) at runtime, making static analysis considerably more complicated. Key entropy is based on the preprocessor macro `__COUNTER__`, the file name(`__FILE__`), and the line number (`__LINE__`) where the variable is defined, and the build time (`__TIME__`) (note: build time is not included when compiling with WDM).

By selecting different encryption levels (Low, Medium, High), you can control the number of encryption rounds. With Low, there are 2 rounds; Medium uses 6; and High adjusts the number of rounds dynamically based on the key entropy, ranging from 6 to 20. This lets you apply lighter encryption to frequently accessed data, and stronger encryption to data that’s used less often.

## Decompilation view
The screenshots show only a small portion of the int main() function. In reality, the function can grow to around 250 lines depending on the compiler.

<table align="center">
<tr>
<td><img src="images/msvc.png" width="400"/></td>
<td><img src="images/llvm.png" width="400"/></td>
<td><img src="images/gcc.png" width="400"/></td>
</tr>
</table>
<p align="center"><em>MSVC, LLVM, GCC compilation (int main()) (Level: Low)</em></p>

## Benchmarks
### Runtime performance impact
| Operation | MSVC | LLVM | GCC |
|-----------|------|------|-----|
| **Integer Operations (Low)** | 3.62 ns | **3.31 ns** ✓ | 4.65 ns (1.4x) |
| **Integer Operations (Medium)** | 10.7 ns (3.0x) | **10.3 ns (3.1x)** ✓ | 17.2 ns (3.7x) |
| **Integer Operations (High)** | 48.3 ns (13.3x) | **41.0 ns (12.4x)** ✓ | 56.1 ns (12.1x) |
| **Float Operations (Low)** | 3.40 ns | **3.26 ns** ✓ | 4.22 ns (1.3x) |
| **Float Operations (Medium)** | 10.8 ns (3.2x) | **10.9 ns (3.3x)** | 16.5 ns (3.9x) |
| **Float Operations (High)** | 46.7 ns (13.7x) | **42.0 ns (12.9x)** ✓ | 56.9 ns (13.5x) |
| **String Operations (Low)** | 36.8 ns | **31.9 ns** ✓ | 43.0 ns (1.3x) |
| **String Operations (Medium)** | 116 ns (3.2x) | **104 ns (3.3x)** ✓ | 174 ns (4.0x) |
| **String Operations (High)** | 495 ns (13.5x) | **429 ns (13.4x)** ✓ | 538 ns (12.5x) |
| **Wide String Operations (Low)** | 31.8 ns | **31.4 ns** ✓ | 47.5 ns (1.5x) |
| **Wide String Operations (Medium)** | 112 ns (3.5x) | **106 ns (3.4x)** ✓ | 172 ns (3.6x) |
| **Wide String Operations (High)** | 503 ns (15.8x) | **417 ns (13.3x)** ✓ | 547 ns (11.5x) |
| **Array Iteration (100 elements, Low)** | 401 ns | **344 ns** ✓ | 436 ns (1.3x) |
| **Array Iteration (100 elements, Medium)** | 1,136 ns (2.8x) | **1,079 ns (3.1x)** ✓ | 1,795 ns (4.1x) |
| **Array Iteration (100 elements, High)** | 5,114 ns (12.8x) | **4,284 ns (12.5x)** ✓ | 5,416 ns (12.4x) |
| **Array Element Access (Low)** | 3.32 ns | **3.21 ns** ✓ | 4.38 ns (1.4x) |
| **Array Element Access (Medium)** | 11.3 ns (3.4x) | **10.2 ns (3.2x)** ✓ | 17.5 ns (4.0x) |
| **Array Element Access (High)** | 49.8 ns (15.0x) | **41.6 ns (13.0x)** ✓ | 56.3 ns (12.9x) |

### Binary size overhead
| Compiler | Without obfuscxx | With obfuscxx | Overhead |
|:---------|:-----------------|:--------------|:---------|
| **MSVC** | 17.0 KB | 18.0 KB | +1,024 bytes (**+5.9%**) |
| **LLVM** | 17.5 KB | 19.6 KB | +1,560 bytes (**+8.7%**) |
| **GCC** | 47.8 KB | 52.2 KB | +4,491 bytes (**+9.2%**) |

**Test environment:**
- CPU: 16 cores @ 2496 MHz
- L1 Data Cache: 48 KiB (x8)
- L1 Instruction Cache: 32 KiB (x8)
- L2 Unified Cache: 512 KiB (x8)
- L3 Unified Cache: 16384 KiB (x1)
- Date: 2025-11-04

## Installation
Just add the header file to your project - `#include "include/obfuscxx.h"`

## Examples
### Basic:
```cpp
#include "include/obfuscxx.h"

int main() {
    obfuscxx<int> int_value{ 100 };
    std::cout << int_value.get() << '\n';
    int_value = 50;
    std::cout << int_value.get() << '\n';

    obfuscxx<float> float_value{ 1.5f };
    std::cout << float_value.get() << '\n';

    obfuscxx<int, 4> array{ 1, 2, 3, 4 };
    for (auto val : array) {
        std::cout << val << " ";
    }
    std::cout << '\n';

    obfuscxx str("Hello, World!");
    std::cout << str.to_string() << '\n';

    obfuscxx<int*> pointer{};
    pointer = new int{101};
    std::cout << pointer.get() << " " << *pointer.get() << '\n';
    delete pointer.get();
}
```

### User-Defined literal (Clang/GCC only):
```cpp
#include "include/obfuscxx.h"

int main() {
    std::cout << "Hello, World!"_obf << '\n';
}
```

### Macros:
```cpp
#include "include/obfuscxx.h"

int main() {
    std::cout << obfusv(42) << " " << obfusv(3.14159f) << '\n';
    std::cout << obfuss("Hello, World!") << '\n';
}
```
## Building tests and benchmarks
1. Install `vcpkg` and set `VCPKG_ROOT` environment variable
2. Fetch baseline: `cd $VCPKG_ROOT && git fetch origin 34823ada10080ddca99b60e85f80f55e18a44eea`
3. Configure: `cmake --preset <compiler>` (MSVC/Clang/GCC)
4. Build: `cmake --build --preset <compiler>` (--config Release/Debug)

## Requirements
- C++20 or later
- Compiler with SIMD support (AVX/SSE/NEON)
- CMake 3.15+ (for building tests)
- vcpkg (for dependencies)

## Compiler support
- `MSVC (+wdm)`
- `Clang`
- `GCC`

## Architecture support
- `x86/x86-64` (SSE/AVX)
- `ARM` (NEON)
