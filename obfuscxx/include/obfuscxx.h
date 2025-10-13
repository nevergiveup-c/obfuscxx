#pragma once

// MIT License
// Copyright (c) 2025 Alexander (nevergiveup-c)
// https://github.com/nevergiveup-c/obfuscxx

#include <cstdint>
#include <initializer_list>

using byte = uint8_t;
using max_align_t = double;

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
#define VOLATILE __attribute__((used))
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

constexpr uint64_t splitmix64(uint64_t x) {
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
#define OBFUSCXX_ENTROPY ( \
    splitmix64( \
        (HASH(__FILE__) * 0x517cc1b727220a95ULL) + \
        ((uint64_t)__LINE__ * 0x9e3779b97f4a7c15ULL) + \
        (rol64((uint64_t)__COUNTER__, 37) ^ ((uint64_t)__LINE__ * 0xff51afd7ed558ccdULL)) \
    ) \
)
#else
#define OBFUSCXX_ENTROPY ( \
    splitmix64( \
        HASH(__FILE__) + \
        ((uint64_t)__LINE__ * 0x9e3779b97f4a7c15ULL) + \
        (HASH(__TIME__) ^ ((uint64_t)__COUNTER__ << 32)) \
    ) \
)
#endif

template<size_t N> FORCEINLINE consteval uint64_t hash_compile_time(char const (&data)[N])
{
	uint64_t hash = 0;

	for (auto i = 0; i < N - 1; ++i)
	{
		hash += data[i] >= 'A' && data[i] <= 'Z' ? data[i] + ('a' - 'A') : data[i];
		hash += hash << 8;
		hash ^= hash >> 11;
	}

	hash += hash << 5;
	hash ^= hash >> 13;
	hash += hash << 10;

	return hash;
}

FORCEINLINE uint64_t hash_runtime(char const* str)
{
	size_t length = 0;
	while (str[length])
		++length;

	uint64_t hash = 0;

	for (auto i = 0u; i < length; i++)
	{
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

constexpr uint64_t rol64(uint64_t x, int n)
{
	n &= 63;
	if (n == 0) return x;
	return (x << n) | (x >> (64 - n));
}

constexpr uint64_t ror64(uint64_t x, int n)
{
	n &= 63;
	if (n == 0) return x;
	return (x >> n) | (x << (64 - n));
}

enum class obf_level : uint8_t { Low, Medium, High };
template <class Type, size_t Size = 1, obf_level Level = obf_level::Low, uint64_t Entropy = OBFUSCXX_ENTROPY>
class obfuscxx
{
	static constexpr bool is_single = Size == 1;
	static constexpr bool is_single_pointer = std::is_pointer_v<Type> && Size == 1;
	static constexpr bool is_char = std::is_same_v<Type, char> || std::is_same_v<Type, const char>;
	static constexpr bool is_wchar = std::is_same_v<Type, wchar_t> || std::is_same_v<Type, const wchar_t>;
	static constexpr bool is_array = Size > 1;

	static constexpr uint64_t seed{ Entropy };
	static constexpr uint64_t iv[8] = {
		0xcbf43b227a01fe5aULL ^ seed,
		0x32703be7aaa7c38fULL ^ ror64(seed, 13),
		0xb589959b3d854bbcULL ^ rol64(seed, 29),
		0x73b3ef5578a97c8aULL ^ ror64(seed, 41),
		0x92afafd27c6e16e9ULL ^ rol64(seed, 7),
		0xee8291ae3070720aULL ^ ror64(seed, 53),
		0xe2c0d70f73d6c4a0ULL ^ rol64(seed, 19),
		0x82742897b912855bULL ^ ror64(seed, 37),
	};
	static constexpr uint64_t iv_size = (sizeof(iv) / 8) - 1;
	static constexpr uint64_t unique_index = seed & iv_size;
	static constexpr uint64_t unique_value = iv[unique_index];

	static constexpr uint32_t xtea_rounds =
		(Level == obf_level::Low) ? 2 :
		(Level == obf_level::Medium) ? 6 :
		(6 + ((unique_index & 0x7) * 2));

	static constexpr uint32_t xtea_delta = (0x9E3779B9 ^ static_cast<uint32_t>(unique_value)) | 1;

	static constexpr uint64_t encrypt(Type value)
	{
		uint64_t val = to_uint64(value);

		uint32_t v0 = static_cast<uint32_t>(val);
		uint32_t v1 = static_cast<uint32_t>(val >> 32);
		uint32_t sum = 0;

		for (uint32_t i = 0; i < xtea_rounds; ++i) {
			v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^
				(sum + static_cast<uint32_t>(iv[sum & 3]));
			sum += xtea_delta;
			v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^
				(sum + static_cast<uint32_t>(iv[(sum >> 11) & 3]));
		}

		return (static_cast<uint64_t>(v1) << 32) | v0;
	}

	static FORCEINLINE Type decrypt(uint64_t value)
	{
		MEM_BARRIER(value)

		uint32_t v0 = static_cast<uint32_t>(value);
		uint32_t v1 = static_cast<uint32_t>(value >> 32);
		uint32_t sum = xtea_delta * xtea_rounds;

#if defined(__aarch64__) || defined(_M_ARM64)
		// ARM64 - NEON
		for (uint32_t i = 0; i < xtea_rounds; ++i) {
			MEM_BARRIER(v0, v1, sum)

			// v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
			uint32x4_t neon_v0 = vdupq_n_u32(v0);
			uint32x4_t neon_left = vshlq_n_u32(neon_v0, 4);
			uint32x4_t neon_right = vshrq_n_u32(neon_v0, 5);
			uint32x4_t neon_temp = veorq_u32(neon_left, neon_right);
			neon_temp = vaddq_u32(neon_temp, neon_v0);

			uint32x4_t neon_key = vdupq_n_u32(sum + static_cast<uint32_t>(iv[(sum >> 11) & 3]));
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

			neon_key = vdupq_n_u32(sum + static_cast<uint32_t>(iv[sum & 3]));
			neon_temp = veorq_u32(neon_temp, neon_key);

			neon_v0 = vdupq_n_u32(v0);
			neon_v0 = vsubq_u32(neon_v0, neon_temp);
			v0 = vgetq_lane_u32(neon_v0, 0);
		}

#elif defined(__clang__) || defined(__GNUC__)
		// x86/x64 GCC/Clang - SSE2
		for (uint32_t i = 0; i < xtea_rounds; ++i) {
			MEM_BARRIER(v0, v1, sum)

			// v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
			__m128i xmm_v0 = _mm_cvtsi32_si128(v0);
			__m128i xmm_left = _mm_slli_epi32(xmm_v0, 4);
			__m128i xmm_right = _mm_srli_epi32(xmm_v0, 5);
			__m128i xmm_temp = _mm_xor_si128(xmm_left, xmm_right);
			xmm_temp = _mm_add_epi32(xmm_temp, xmm_v0);

			__m128i xmm_key = _mm_cvtsi32_si128(sum + static_cast<uint32_t>(iv[(sum >> 11) & 3]));
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

			xmm_key = _mm_cvtsi32_si128(sum + static_cast<uint32_t>(iv[sum & 3]));
			xmm_temp = _mm_xor_si128(xmm_temp, xmm_key);

			xmm_v0 = _mm_cvtsi32_si128(v0);
			xmm_v0 = _mm_sub_epi32(xmm_v0, xmm_temp);
			v0 = _mm_cvtsi128_si32(xmm_v0);
		}

#else
		// MSVC x86/x64 - AVX2
		for (uint32_t i = 0; i < xtea_rounds; ++i) {
			MEM_BARRIER(v0, v1, sum)

			// v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key)
			__m256i mm256_v0 = _mm256_set1_epi32(v0);
			__m256i mm256_left = _mm256_slli_epi32(mm256_v0, 4);
			__m256i mm256_right = _mm256_srli_epi32(mm256_v0, 5);
			__m256i mm256_temp = _mm256_xor_si256(mm256_left, mm256_right);
			mm256_temp = _mm256_add_epi32(mm256_temp, mm256_v0);

			__m256i mm256_key = _mm256_set1_epi32(sum + static_cast<uint32_t>(iv[(sum >> 11) & 3]));
			mm256_temp = _mm256_xor_si256(mm256_temp, mm256_key);

			__m256i mm256_v1 = _mm256_set1_epi32(v1);
			mm256_v1 = _mm256_sub_epi32(mm256_v1, mm256_temp);
			v1 = _mm256_extract_epi32(mm256_v1, 0);

			sum -= xtea_delta;

			MEM_BARRIER(v0, v1, sum)

			// v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key)
			mm256_v1 = _mm256_set1_epi32(v1);
			mm256_left = _mm256_slli_epi32(mm256_v1, 4);
			mm256_right = _mm256_srli_epi32(mm256_v1, 5);
			mm256_temp = _mm256_xor_si256(mm256_left, mm256_right);
			mm256_temp = _mm256_add_epi32(mm256_temp, mm256_v1);

			mm256_key = _mm256_set1_epi32(sum + static_cast<uint32_t>(iv[sum & 3]));
			mm256_temp = _mm256_xor_si256(mm256_temp, mm256_key);

			mm256_v0 = _mm256_set1_epi32(v0);
			mm256_v0 = _mm256_sub_epi32(mm256_v0, mm256_temp);
			v0 = _mm256_extract_epi32(mm256_v0, 0);
		}
#endif

		return from_uint64((static_cast<uint64_t>(v1) << 32) | v0);
	}

	static constexpr uint64_t to_uint64(Type value)
	{
		if constexpr (std::is_pointer_v<Type>) {
			return reinterpret_cast<uint64_t>(value);
		}
		else if constexpr (std::is_floating_point_v<Type>) {
			if constexpr (sizeof(Type) == 4) {
				return __builtin_bit_cast(uint32_t, value);
			}
			else {
				return __builtin_bit_cast(uint64_t, value);
			}
		}
		else {
			return static_cast<uint64_t>(value);
		}
	}
	static FORCEINLINE Type from_uint64(uint64_t value)
	{
		if constexpr (std::is_pointer_v<Type>) {
			return reinterpret_cast<Type>(value);
		}
		else if constexpr (std::is_floating_point_v<Type>) {
			if constexpr (sizeof(Type) == 4) {
				return __builtin_bit_cast(Type, static_cast<uint32_t>(value));
			}
			else {
				return __builtin_bit_cast(Type, value);
			}
		}
		else {
			return static_cast<Type>(value);
		}
	}

public:
	explicit consteval obfuscxx(Type val)
	{
		data[0] = encrypt(val);
	}
	explicit consteval obfuscxx(const std::initializer_list<Type>& list)
	{
		for (size_t i{}; const auto& v : list)
			data[i++] = encrypt(v);
	}
	explicit consteval obfuscxx(Type(&val)[Size])
	{
		for (size_t i{}; i < Size; ++i) {
			data[i] = encrypt(val[i]);
		}
	}
	explicit consteval obfuscxx(const Type(&val)[Size])
	{
		for (size_t i{}; i < Size; ++i) {
			data[i] = encrypt(val[i]);
		}
	}

	FORCEINLINE Type get() const requires is_single
	{
		volatile const uint64_t* ptr = &data[0];
		uint64_t val = *ptr;
		return decrypt(val);
	}
	FORCEINLINE Type get(size_t i) const requires is_array
	{
		volatile const uint64_t* ptr = &data[i];
		uint64_t val = *ptr;
		return decrypt(val);
	}

	FORCEINLINE void copy_to(Type* out, size_t count) const requires is_array
	{
		size_t n = (count < Size) ? count : Size;
		for (size_t i = 0; i < n; ++i) {
			volatile const uint64_t* ptr = &data[i];
			out[i] = decrypt(*ptr);
		}
	}

	FORCEINLINE void set(Type val) requires is_single
	{
		data[0] = encrypt(val);
	}
	FORCEINLINE void set(Type val, size_t i) requires is_array
	{
		data[i] = encrypt(val);
	}
	FORCEINLINE void set(const std::initializer_list<Type>& list) requires is_array
	{
		for (size_t i{}; const auto& val : list) {
			if (i < Size) {
				data[i++] = encrypt(val);
			}
		}
	}

	FORCEINLINE Type operator()() const requires is_single
	{
		return decrypt(data[0]);
	}
	FORCEINLINE Type operator[](size_t i) const requires is_array
	{
		return decrypt(data[i]);
	}

	FORCEINLINE obfuscxx& operator=(Type val) requires is_single
	{
		set(val);
		return *this;
	}
	FORCEINLINE obfuscxx& operator=(const std::initializer_list<Type>& list) requires is_array
	{
		set(list);
		return *this;
	}

	FORCEINLINE bool operator==(const obfuscxx& rhs) const requires is_single
	{
		return get() == rhs.get();
	}
	FORCEINLINE bool operator==(const obfuscxx& rhs) const requires is_array
	{
		for (size_t i = 0; i < Size; ++i) {
			if (get(i) != rhs.get(i)) {
				return false;
			}
		}
		return true;
	}
	FORCEINLINE bool operator!=(const obfuscxx& rhs) const
	{
		return !(*this == rhs);
	}

	FORCEINLINE operator Type() const requires is_single
	{
		return get();
	}

	FORCEINLINE bool operator<(const obfuscxx& rhs) const
	{
		return get() < rhs.get();
	}
	FORCEINLINE bool operator>(const obfuscxx& rhs) const
	{
		return get() > rhs.get();
	}
	FORCEINLINE bool operator<=(const obfuscxx& rhs) const
	{
		return get() <= rhs.get();
	}
	FORCEINLINE bool operator>=(const obfuscxx& rhs) const
	{
		return get() >= rhs.get();
	}

	FORCEINLINE Type operator+(const obfuscxx& rhs) const
	{
		return get() + rhs.get();
	}
	FORCEINLINE Type operator-(const obfuscxx& rhs) const
	{
		return get() - rhs.get();
	}
	FORCEINLINE Type operator*(const obfuscxx& rhs) const requires (!is_single_pointer)
	{
		return get() * rhs.get();
	}
	FORCEINLINE Type operator/(const obfuscxx& rhs) const requires (!is_single_pointer)
	{
		return get() / rhs.get();
	}

	FORCEINLINE obfuscxx& operator+=(const obfuscxx& rhs) requires (!is_single_pointer)
	{
		set(get() + rhs.get());
		return *this;
	}
	FORCEINLINE obfuscxx& operator-=(const obfuscxx& rhs) requires (!is_single_pointer)
	{
		set(get() - rhs.get());
		return *this;
	}

	FORCEINLINE Type operator->() requires is_single_pointer
	{
		return get();
	}
	FORCEINLINE Type& operator*() requires is_single_pointer
	{
		return *get();
	}

	struct iterator {
		const obfuscxx* parent;
		size_t index;

		Type operator*() const { return parent->get(index); }
		iterator& operator++() { ++index; return *this; }
		bool operator!=(const iterator& other) const { return index != other.index; }
		bool operator==(const iterator& other) const { return index == other.index; }
	};

	iterator begin() const requires is_array { return { this, 0 }; }
	iterator end() const requires is_array { return { this, Size }; }
	static constexpr size_t size() { return Size; }

	template<class CharType, size_t N> struct string_copy {
		CharType data[N];

		static constexpr bool is_char = std::is_same_v<CharType, char> ||
			std::is_same_v<CharType, const char>;
		static constexpr bool is_wchar = std::is_same_v<CharType, wchar_t> ||
			std::is_same_v<CharType, const wchar_t>;

		operator const char* () const requires is_char { return data; }
		operator const wchar_t* () const requires is_wchar { return data; }
		const CharType* c_str() const { return data; }
	};

	FORCEINLINE string_copy<Type, Size> to_string() const requires (is_char || is_wchar)
	{
		string_copy<Type, Size> result{};
		copy_to(result.data, Size);
		return result;
	}

private:
	VOLATILE uint64_t data[Size]{};
};