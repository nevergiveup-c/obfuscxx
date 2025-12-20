#pragma once

// MIT License
// Copyright (c) 2025 Alexander (nevergiveup-c)
// https://github.com/nevergiveup-c/obfuscxx

#include <cstdint>
#include <initializer_list>

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
using byte = std::uint8_t;
using max_align_t = double;
#endif

#include <type_traits>

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__clang__) || defined(__GNUC__)
#include <immintrin.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#else
#include <immintrin.h>
#endif

#if defined(__clang__) || defined(__GNUC__)
#define VOLATILE
#elif defined(_MSC_VER)
#define VOLATILE volatile
#endif

#if defined(__clang__) || defined(__GNUC__)
#define FORCEINLINE __attribute__((always_inline)) inline
#else
#define FORCEINLINE __forceinline
#endif

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
#define _mm256_extract_epi32(vec, idx) (((int32_t*)&(vec))[(idx)])
#endif

#if defined(__clang__) || defined(__GNUC__)
#define MEM_BARRIER(...) __asm__ volatile("" : "+r"(__VA_ARGS__) :: "memory");
#elif defined(_MSC_VER)
#define MEM_BARRIER(...) _ReadWriteBarrier();
#endif

#ifndef OBFUSCXX_DISABLE_WARNS
#define RUNTIME_WARNING [[deprecated("OBFUSCXX: Runtime set() uses encrypt method without SIMD obfuscation. For better protection, initialize at compile-time.")]]
#else
#define RUNTIME_WARNING
#endif

constexpr std::uint64_t splitmix64(std::uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
#define OBFUSCXX_ENTROPY ( \
    splitmix64( \
        (HASH(__FILE__) * 0x517cc1b727220a95ULL) + \
        ((std::uint64_t)__LINE__ * 0x9e3779b97f4a7c15ULL) + \
        (rol64((std::uint64_t)__COUNTER__, 37) ^ ((std::uint64_t)__LINE__ * 0xff51afd7ed558ccdULL)) \
    ) \
)
#else
#define OBFUSCXX_ENTROPY ( \
    splitmix64( \
        HASH(__FILE__) + \
        ((std::uint64_t)__LINE__ * 0x9e3779b97f4a7c15ULL) + \
        (HASH(__TIME__) ^ ((std::uint64_t)__COUNTER__ << 32)) \
    ) \
)
#endif

template<std::size_t N>
FORCEINLINE consteval std::uint64_t hash_compile_time(char const (&data)[N]) {
    std::uint64_t hash = 0;

    for (auto i = 0; i < N - 1; ++i) {
        hash += data[i] >= 'A' && data[i] <= 'Z' ? data[i] + ('a' - 'A') : data[i];
        hash += hash << 8;
        hash ^= hash >> 11;
    }

    hash += hash << 5;
    hash ^= hash >> 13;
    hash += hash << 10;

    return hash;
}

FORCEINLINE std::uint64_t hash_runtime(char const *str) {
    std::size_t length = 0;
    while (str[length])
        ++length;

    std::uint64_t hash = 0;

    for (auto i = 0u; i < length; i++) {
        hash += str[i] >= 'A' && str[i] <= 'Z' ? str[i] + ('a' - 'A') : str[i];
        hash += hash << 8;
        hash ^= hash >> 11;
    }

    hash += hash << 5;
    hash ^= hash >> 13;
    hash += hash << 10;

    return hash;
}

#define HASH( s ) hash_compile_time( s )
#define HASH_RT( s ) hash_runtime( s )

constexpr std::uint64_t rol64(std::uint64_t x, int n) {
    n &= 63;
    if (n == 0) return x;
    return (x << n) | (x >> (64 - n));
}

constexpr std::uint64_t ror64(std::uint64_t x, int n) {
    n &= 63;
    if (n == 0) return x;
    return (x >> n) | (x << (64 - n));
}

enum class obf_level : std::uint8_t { Low, Medium, High };

template<class Type, std::size_t Size = 1, obf_level Level = obf_level::Low, std::uint64_t Entropy = OBFUSCXX_ENTROPY>
class obfuscxx {
    static constexpr bool is_single = Size == 1;
    static constexpr bool is_array = Size > 1;
    static constexpr bool is_single_pointer = std::is_pointer_v<Type> && Size == 1;
    static constexpr bool is_char = std::is_same_v<Type, char> || std::is_same_v<Type, const char>;
    static constexpr bool is_wchar = std::is_same_v<Type, wchar_t> || std::is_same_v<Type, const wchar_t>;

    struct passkey { explicit passkey() = default; };

    static constexpr std::uint64_t seed{Entropy};
    static constexpr std::uint64_t iv[8] = {
        0xcbf43b227a01fe5aULL ^ seed,
        0x32703be7aaa7c38fULL ^ ror64(seed, 13),
        0xb589959b3d854bbcULL ^ rol64(seed, 29),
        0x73b3ef5578a97c8aULL ^ ror64(seed, 41),
        0x92afafd27c6e16e9ULL ^ rol64(seed, 7),
        0xee8291ae3070720aULL ^ ror64(seed, 53),
        0xe2c0d70f73d6c4a0ULL ^ rol64(seed, 19),
        0x82742897b912855bULL ^ ror64(seed, 37),
    };
    static constexpr std::uint64_t iv_size = (sizeof(iv) / 8) - 1;
    static constexpr std::uint64_t unique_index = seed & iv_size;
    static constexpr std::uint64_t unique_value = iv[unique_index];

    static constexpr std::uint32_t xtea_rounds =
            (Level == obf_level::Low) ? 2 : (Level == obf_level::Medium) ? 6 : (6 + ((unique_index & 0x7) * 2));

    static constexpr std::uint32_t xtea_delta = (0x9E3779B9 ^ static_cast<std::uint32_t>(unique_value)) | 1;

    static constexpr std::uint64_t encrypt(Type value) {
        std::uint64_t val = to_uint64(value);

        std::uint32_t v0 = static_cast<std::uint32_t>(val);
        std::uint32_t v1 = static_cast<std::uint32_t>(val >> 32);
        std::uint32_t sum = 0;

        for (std::uint32_t i = 0; i < xtea_rounds; ++i) {
            v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^
                    (sum + static_cast<std::uint32_t>(iv[sum & 3]));
            sum += xtea_delta;
            v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^
                    (sum + static_cast<std::uint32_t>(iv[(sum >> 11) & 3]));
        }

        return (static_cast<std::uint64_t>(v1) << 32) | v0;
    }

    static FORCEINLINE Type decrypt(std::uint64_t value) {
        MEM_BARRIER(value)

        std::uint32_t v0 = static_cast<std::uint32_t>(value);
        std::uint32_t v1 = static_cast<std::uint32_t>(value >> 32);
        std::uint32_t sum = xtea_delta * xtea_rounds;

#if defined(__aarch64__) || defined(_M_ARM64)
        // ARM64 - NEON
        for (std::uint32_t i = 0; i < xtea_rounds; ++i) {
            MEM_BARRIER(v0, v1, sum)

            // v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
            uint32x4_t neon_v0 = vdupq_n_u32(v0);
            uint32x4_t neon_left = vshlq_n_u32(neon_v0, 4);
            uint32x4_t neon_right = vshrq_n_u32(neon_v0, 5);
            uint32x4_t neon_temp = veorq_u32(neon_left, neon_right);
            neon_temp = vaddq_u32(neon_temp, neon_v0);

            uint32x4_t neon_key = vdupq_n_u32(sum + static_cast<std::uint32_t>(iv[(sum >> 11) & 3]));
            neon_temp = veorq_u32(neon_temp, neon_key);

            uint32x4_t neon_v1 = vdupq_n_u32(v1);
            neon_v1 = vsubq_u32(neon_v1, neon_temp);
            v1 = vgetq_lane_u32(neon_v1, 0);

            sum -= xtea_delta;

            MEM_BARRIER(v0, v1, sum)

            // v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key)
            neon_v1 = vdupq_n_u32(v1);
            neon_left = vshlq_n_u32(neon_v1, 4);
            neon_right = vshrq_n_u32(neon_v1, 5);
            neon_temp = veorq_u32(neon_left, neon_right);
            neon_temp = vaddq_u32(neon_temp, neon_v1);

            neon_key = vdupq_n_u32(sum + static_cast<std::uint32_t>(iv[sum & 3]));
            neon_temp = veorq_u32(neon_temp, neon_key);

            neon_v0 = vdupq_n_u32(v0);
            neon_v0 = vsubq_u32(neon_v0, neon_temp);
            v0 = vgetq_lane_u32(neon_v0, 0);
        }

#elif defined(__clang__) || defined(__GNUC__)
        // x86/x64 GCC/Clang - SSE2
        for (std::uint32_t i = 0; i < xtea_rounds; ++i) {
            MEM_BARRIER(v0, v1, sum)

            // v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
            __m128i xmm_v0 = _mm_cvtsi32_si128(v0);
            __m128i xmm_left = _mm_slli_epi32(xmm_v0, 4);
            __m128i xmm_right = _mm_srli_epi32(xmm_v0, 5);
            __m128i xmm_temp = _mm_xor_si128(xmm_left, xmm_right);
            xmm_temp = _mm_add_epi32(xmm_temp, xmm_v0);

            __m128i xmm_key = _mm_cvtsi32_si128(sum + static_cast<std::uint32_t>(iv[(sum >> 11) & 3]));
            xmm_temp = _mm_xor_si128(xmm_temp, xmm_key);

            __m128i xmm_v1 = _mm_cvtsi32_si128(v1);
            xmm_v1 = _mm_sub_epi32(xmm_v1, xmm_temp);
            v1 = _mm_cvtsi128_si32(xmm_v1);

            sum -= xtea_delta;

            MEM_BARRIER(v0, v1, sum)

            // v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key)
            xmm_v1 = _mm_cvtsi32_si128(v1);
            xmm_left = _mm_slli_epi32(xmm_v1, 4);
            xmm_right = _mm_srli_epi32(xmm_v1, 5);
            xmm_temp = _mm_xor_si128(xmm_left, xmm_right);
            xmm_temp = _mm_add_epi32(xmm_temp, xmm_v1);

            xmm_key = _mm_cvtsi32_si128(sum + static_cast<std::uint32_t>(iv[sum & 3]));
            xmm_temp = _mm_xor_si128(xmm_temp, xmm_key);

            xmm_v0 = _mm_cvtsi32_si128(v0);
            xmm_v0 = _mm_sub_epi32(xmm_v0, xmm_temp);
            v0 = _mm_cvtsi128_si32(xmm_v0);
        }

#else

        for (std::uint32_t i = 0; i < xtea_rounds; ++i) {
            MEM_BARRIER(v0, v1, sum)

            // v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
            if (cpu_has_avx2()) {
                __m256i mm256_v0 = _mm256_set1_epi32(v0);
                __m256i mm256_left = _mm256_slli_epi32(mm256_v0, 4);
                __m256i mm256_right = _mm256_srli_epi32(mm256_v0, 5);
                __m256i mm256_temp = _mm256_xor_si256(mm256_left, mm256_right);
                mm256_temp = _mm256_add_epi32(mm256_temp, mm256_v0);

                __m256i mm256_key = _mm256_set1_epi32(sum + static_cast<std::uint32_t>(iv[(sum >> 11) & 3]));
                mm256_temp = _mm256_xor_si256(mm256_temp, mm256_key);

                __m256i mm256_v1 = _mm256_set1_epi32(v1);
                mm256_v1 = _mm256_sub_epi32(mm256_v1, mm256_temp);
                v1 = _mm256_extract_epi32(mm256_v1, 0);
            } else {
                __m128i mm128_v0 = _mm_set1_epi32(v0);
                __m128i mm128_left = _mm_slli_epi32(mm128_v0, 4);
                __m128i mm128_right = _mm_srli_epi32(mm128_v0, 5);
                __m128i mm128_temp = _mm_xor_si128(mm128_left, mm128_right);
                mm128_temp = _mm_add_epi32(mm128_temp, mm128_v0);

                __m128i mm128_key = _mm_set1_epi32(sum + static_cast<std::uint32_t>(iv[(sum >> 11) & 3]));
                mm128_temp = _mm_xor_si128(mm128_temp, mm128_key);

                __m128i mm128_v1 = _mm_set1_epi32(v1);
                mm128_v1 = _mm_sub_epi32(mm128_v1, mm128_temp);
                v1 = _mm_extract_epi32(mm128_v1, 0);
            }

            sum -= xtea_delta;

            MEM_BARRIER(v0, v1, sum)

            // v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key)
            if (cpu_has_avx2()) {
                __m256i mm256_v1 = _mm256_set1_epi32(v1);
                __m256i mm256_left = _mm256_slli_epi32(mm256_v1, 4);
                __m256i mm256_right = _mm256_srli_epi32(mm256_v1, 5);
                __m256i mm256_temp = _mm256_xor_si256(mm256_left, mm256_right);
                mm256_temp = _mm256_add_epi32(mm256_temp, mm256_v1);

                __m256i mm256_key = _mm256_set1_epi32(sum + static_cast<std::uint32_t>(iv[sum & 3]));
                mm256_temp = _mm256_xor_si256(mm256_temp, mm256_key);

                __m256i mm256_v0 = _mm256_set1_epi32(v0);
                mm256_v0 = _mm256_sub_epi32(mm256_v0, mm256_temp);
                v0 = _mm256_extract_epi32(mm256_v0, 0);
            } else {
                __m128i mm128_v1 = _mm_set1_epi32(v1);
                __m128i mm128_left = _mm_slli_epi32(mm128_v1, 4);
                __m128i mm128_right = _mm_srli_epi32(mm128_v1, 5);
                __m128i mm128_temp = _mm_xor_si128(mm128_left, mm128_right);
                mm128_temp = _mm_add_epi32(mm128_temp, mm128_v1);

                __m128i mm128_key = _mm_set1_epi32(sum + static_cast<std::uint32_t>(iv[sum & 3]));
                mm128_temp = _mm_xor_si128(mm128_temp, mm128_key);

                __m128i mm128_v0 = _mm_set1_epi32(v0);
                mm128_v0 = _mm_sub_epi32(mm128_v0, mm128_temp);
                v0 = _mm_extract_epi32(mm128_v0, 0);
            }
        }

#endif

        return from_uint64((static_cast<std::uint64_t>(v1) << 32) | v0);
    }

    static constexpr std::uint64_t to_uint64(Type value) {
        if constexpr (std::is_pointer_v<Type>) {
            return reinterpret_cast<std::uint64_t>(value);
        } else if constexpr (std::is_floating_point_v<Type>) {
            if constexpr (sizeof(Type) == 4) {
                return __builtin_bit_cast(std::uint32_t, value);
            } else {
                return __builtin_bit_cast(std::uint64_t, value);
            }
        } else {
            return static_cast<std::uint64_t>(value);
        }
    }

    static FORCEINLINE Type from_uint64(std::uint64_t value) {
        if constexpr (std::is_pointer_v<Type>) {
            return reinterpret_cast<Type>(value);
        } else if constexpr (std::is_floating_point_v<Type>) {
            if constexpr (sizeof(Type) == 4) {
                return __builtin_bit_cast(Type, static_cast<std::uint32_t>(value));
            } else {
                return __builtin_bit_cast(Type, value);
            }
        } else {
            return static_cast<Type>(value);
        }
    }

    static FORCEINLINE bool cpu_has_avx2() {
        int cpuInfo[4]{};
        __cpuidex(cpuInfo, 7, 0);
        return (cpuInfo[1] & (1 << 5)) != 0;
    }

public:
    explicit consteval obfuscxx(passkey) {
        for (std::size_t i = 0; i < Size; i++) {
            data[i] = seed ^ iv[i & iv_size];
        }
    }

    explicit consteval obfuscxx(Type val) : obfuscxx(passkey{}) {
        data[0] = encrypt(val);
    }

    explicit consteval obfuscxx(Type (&val)[Size]) : obfuscxx(passkey{}) {
        for (std::size_t i{}; i < Size; ++i) {
            data[i] = encrypt(val[i]);
        }
    }

    explicit consteval obfuscxx(const Type (&val)[Size]) : obfuscxx(passkey{}) {
        for (std::size_t i{}; i < Size; ++i) {
            data[i] = encrypt(val[i]);
        }
    }

    consteval obfuscxx(const std::initializer_list<Type> &list) : obfuscxx(passkey{}) {
        for (std::size_t i{}; const auto &v: list)
            data[i++] = encrypt(v);
    }

    FORCEINLINE Type get() const requires is_single {
        volatile const std::uint64_t *ptr = &data[0];
        std::uint64_t val = *ptr;
        return decrypt(val);
    }

    FORCEINLINE Type get(std::size_t i) const requires is_array {
        volatile const std::uint64_t *ptr = &data[i];
        std::uint64_t val = *ptr;
        return decrypt(val);
    }

    FORCEINLINE void copy_to(Type *out, std::size_t count) const requires is_array {
        std::size_t n = (count < Size) ? count : Size;
        for (std::size_t i = 0; i < n; ++i) {
            volatile const std::uint64_t *ptr = &data[i];
            out[i] = decrypt(*ptr);
        }
    }

    RUNTIME_WARNING FORCEINLINE void set(Type val) requires is_single {
        data[0] = encrypt(val);
    }

    RUNTIME_WARNING FORCEINLINE void set(Type val, std::size_t i) requires is_array {
        data[i] = encrypt(val);
    }

    RUNTIME_WARNING FORCEINLINE void set(const std::initializer_list<Type> &list) requires is_array {
        for (std::size_t i{}; const auto &val: list) {
            if (i < Size) {
                data[i++] = encrypt(val);
            }
        }
    }

    FORCEINLINE Type operator()() const requires is_single {
        return decrypt(data[0]);
    }

    FORCEINLINE Type operator[](std::size_t i) const requires is_array {
        return decrypt(data[i]);
    }

    FORCEINLINE obfuscxx &operator=(Type val) requires is_single {
        set(val);
        return *this;
    }

    FORCEINLINE obfuscxx &operator=(const std::initializer_list<Type> &list) requires is_array {
        set(list);
        return *this;
    }

    FORCEINLINE bool operator==(const obfuscxx &rhs) const requires is_single {
        return get() == rhs.get();
    }

    FORCEINLINE bool operator==(const obfuscxx &rhs) const requires is_array {
        for (std::size_t i = 0; i < Size; ++i) {
            if (get(i) != rhs.get(i)) {
                return false;
            }
        }
        return true;
    }

    FORCEINLINE bool operator!=(const obfuscxx &rhs) const {
        return !(*this == rhs);
    }

    FORCEINLINE operator Type() const requires is_single {
        return get();
    }

    FORCEINLINE bool operator<(const obfuscxx &rhs) const {
        return get() < rhs.get();
    }

    FORCEINLINE bool operator>(const obfuscxx &rhs) const {
        return get() > rhs.get();
    }

    FORCEINLINE bool operator<=(const obfuscxx &rhs) const {
        return get() <= rhs.get();
    }

    FORCEINLINE bool operator>=(const obfuscxx &rhs) const {
        return get() >= rhs.get();
    }

    FORCEINLINE Type operator+(const obfuscxx &rhs) const {
        return get() + rhs.get();
    }

    FORCEINLINE Type operator-(const obfuscxx &rhs) const {
        return get() - rhs.get();
    }

    FORCEINLINE Type operator*(const obfuscxx &rhs) const requires (!is_single_pointer) {
        return get() * rhs.get();
    }

    FORCEINLINE Type operator/(const obfuscxx &rhs) const requires (!is_single_pointer) {
        return get() / rhs.get();
    }

    FORCEINLINE obfuscxx &operator+=(const obfuscxx &rhs) requires (!is_single_pointer) {
        set(get() + rhs.get());
        return *this;
    }

    FORCEINLINE obfuscxx &operator-=(const obfuscxx &rhs) requires (!is_single_pointer) {
        set(get() - rhs.get());
        return *this;
    }

    FORCEINLINE Type operator->() requires is_single_pointer {
        return get();
    }

    FORCEINLINE Type &operator*() requires is_single_pointer {
        return *get();
    }

    struct iterator {
        const obfuscxx *parent;
        std::size_t index;

        Type operator*() const { return parent->get(index); }

        iterator &operator++() {
            ++index;
            return *this;
        }

        bool operator!=(const iterator &other) const { return index != other.index; }
        bool operator==(const iterator &other) const { return index == other.index; }
    };

    iterator begin() const requires is_array { return {this, 0}; }
    iterator end() const requires is_array { return {this, Size}; }
    static constexpr std::size_t size() { return Size; }

    template<class CharType, std::size_t N>
    struct string_copy {
    private:
        static constexpr bool is_char = std::is_same_v<CharType, char> ||
                                        std::is_same_v<CharType, const char>;
        static constexpr bool is_wchar = std::is_same_v<CharType, wchar_t> ||
                                         std::is_same_v<CharType, const wchar_t>;

    public:
        operator const char *() const requires is_char { return data; }
        operator const wchar_t *() const requires is_wchar { return data; }

        const CharType *c_str() const { return data; }
        const CharType &operator[](std::size_t i) const { return data[i]; }

        const CharType *begin() const { return data; }
        const CharType *end() const { return data + N; }

        constexpr std::size_t size() const { return N; }

        CharType data[N];
    };

    template<class ArrayType, std::size_t N>
    struct array_copy {
        const ArrayType *get() const { return data; }

        const ArrayType *begin() const { return data; }
        const ArrayType *end() const { return data + N; }

        constexpr std::size_t size() const { return N; }
        constexpr std::size_t size_bytes() const { return N * sizeof(ArrayType); }

        ArrayType data[N];
    };

    FORCEINLINE string_copy<Type, Size> to_string() const requires (is_char || is_wchar) {
        string_copy<Type, Size> result{};
        if constexpr (is_array) {
            copy_to(result.data, Size);
        } else {
            result.data[0] = decrypt(data[0]);
        }
        return result;
    }

    FORCEINLINE array_copy<Type, Size> to_array() const requires (is_array) {
        array_copy<Type, Size> result{};
        if constexpr (is_array) {
            copy_to(result.data, Size);
        } else {
            result.data[0] = decrypt(data[0]);
        }
        return result;
    }

private:
    VOLATILE std::uint64_t data[Size]{};
};

#if defined(__clang__) || defined(__GNUC__)
template<typename CharType, CharType... chars>
constexpr auto operator""_obf() {
    constexpr CharType str[] = {chars..., '\0'};
    return obfuscxx(str).to_string();
}
#endif

#define obfusv(val) obfuscxx(val).get()
#define obfuss(str) obfuscxx(str).to_string().c_str()