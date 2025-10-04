#pragma once
#include <cstdint>
#include <initializer_list>
#include <type_traits>

#if defined(__clang__) || defined(__GNUC__)
#include <immintrin.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#else
#include <immintrin.h>
#endif

#ifdef __clang__
#define VOLATILE __attribute__((used))
#elif defined(_MSC_VER)
#define VOLATILE volatile
#endif

#if defined(__clang__) || defined(__GNUC__)
#define FORCEINLINE inline __attribute__((always_inline))
#else
#define FORCEINLINE __forceinline
#endif

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
#define _mm256_extract_epi64(vec, idx) _mm_cvtsi128_si64(_mm256_castsi256_si128(vec))
#endif

#if defined(_KERNEL_MODE) || defined(_WIN64_DRIVER)
#define OBFUSCXX_ENTROPY (((uint64_t)(HASH(__FILE__) * __LINE__) ^ (0xDEADBEAFULL * __LINE__)) * (uint64_t)((__COUNTER__ << 16)) & 0x7FFFFFFFFFFFFFFFULL)
#else
#define OBFUSCXX_ENTROPY (((uint64_t)(HASH(__FILE__) * __LINE__) ^ (HASH(__TIME__) * __LINE__)) * (uint64_t)((__COUNTER__ << 16)) & 0x7FFFFFFFFFFFFFFFULL)
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
	return (x << n) | (x >> (64 - n));
}

constexpr uint64_t ror64(uint64_t x, int n)
{
	n &= 63;
	return (x >> n) | (x << (64 - n));
}

FORCEINLINE uint64_t ror64_avx(uint64_t v, int count)
{
	__m128i xmm_val = _mm_set1_epi64x(v);
	__m128i shifted_r = _mm_srli_epi64(xmm_val, count);
	__m128i shifted_l = _mm_slli_epi64(xmm_val, 64 - count);
	__m128i result = _mm_or_si128(shifted_r, shifted_l);
	return _mm_cvtsi128_si64(result);
}

FORCEINLINE uint64_t rol64_avx(uint64_t v, int count)
{
	__m128i xmm_val = _mm_set1_epi64x(v);
	__m128i shifted_l = _mm_slli_epi64(xmm_val, count);
	__m128i shifted_r = _mm_srli_epi64(xmm_val, 64 - count);
	__m128i result = _mm_or_si128(shifted_l, shifted_r);
	return _mm_cvtsi128_si64(result);
}

enum class obf_level : uint8_t { Low, High };
template <class Type, size_t Size = 1, obf_level Level = obf_level::High, uint64_t Entropy = OBFUSCXX_ENTROPY>
class obfuscxx
{
	static constexpr bool single = Size == 1;
	static constexpr bool single_pointer = std::is_pointer_v<Type> && Size == 1;
	static constexpr bool array = Size > 1;

	static constexpr uint64_t seed{ Entropy };
	static constexpr uint64_t iv[8] = {
		0xcbf43b227a01fe5a ^ rol64(seed, 4),
		0x32703be7aaa7c38f ^ rol64(seed, 8),
		0xb589959b3d854bbc ^ ror64(seed, 16),
		0x73b3ef5578a97c8a ^ ror64(seed, 24),
		0x92afafd27c6e16e9 ^ rol64(seed, 32),
		0xee8291ae3070720a ^ rol64(seed, 40),
		0xe2c0d70f73d6c4a0 ^ ror64(seed, 48),
		0x82742897b912855b ^ rol64(seed, 56),
	};

	static constexpr uint64_t encrypt(Type value)
	{
		uint64_t val = to_uint64(value);

		if constexpr (Level == obf_level::Low)
		{
			return rol64(val ^ iv[0], (iv[7] & 0xF));
		}

		val = rol64(val, (iv[1] & 0xF));
		val = ror64(val, (iv[4] & 0xF));

		for (const uint64_t key : iv) {
			val = rol64(val, (iv[2] & 0xF));
			val ^= key;
			val = ror64(val, (iv[5] & 0xF));
		}

		val = ror64(val, (iv[3] & 0xF));
		val = rol64(val, (iv[2] & 0xF));

		return val;
	}

	static FORCEINLINE Type decrypt(uint64_t val)
	{
#if defined(__GNUC__) || defined(__clang__)
		__asm__ volatile("" : "+r"(val) :: "memory");
#elif defined(_MSC_VER)
		_ReadWriteBarrier();
#endif

		if constexpr (Level == obf_level::Low)
		{
			return from_uint64(ror64_avx(val, (iv[7] & 0xF)) ^ iv[0]);
		}
#if defined(__clang__) || defined(__GNUC__)
		val = ror64_avx(val, (iv[2] & 0xF));
		val = rol64_avx(val, (iv[3] & 0xF));

		for (int i = 8; i >= 1; --i) {
			val = rol64_avx(val, (iv[5] & 0xF));

			__m128i xmm_val = _mm_set_epi64x(0, val);
			__m128i xmm_key = _mm_set_epi64x(0, iv[i - 1]);
			xmm_val = _mm_xor_si128(xmm_val, xmm_key);
			val = _mm_cvtsi128_si64(xmm_val);

			val = ror64_avx(val, (iv[2] & 0xF));
		}

		val = rol64_avx(val, (iv[4] & 0xF));
		val = ror64_avx(val, (iv[1] & 0xF));

		return from_uint64(val);
#else
		__m256i mm256 = _mm256_set1_epi64x(val);

		mm256 = _mm256_ror_epi64(mm256, (iv[2] & 0xF));
		mm256 = _mm256_rol_epi64(mm256, (iv[3] & 0xF));
		val = _mm256_extract_epi64(mm256, 0);

		for (int i = 8; i >= 1; --i) {
			mm256 = _mm256_set1_epi64x(val);
			mm256 = _mm256_rol_epi64(mm256, (iv[5] & 0xF));
			val = _mm256_extract_epi64(mm256, 0);

			__m128i xmm_val = _mm_set_epi64x(0, val);
			__m128i xmm_key = _mm_set_epi64x(0, iv[i - 1]);
			xmm_val = _mm_xor_si128(xmm_val, xmm_key);
			val = _mm_cvtsi128_si64(xmm_val);

			mm256 = _mm256_set1_epi64x(val);
			mm256 = _mm256_ror_epi64(mm256, (iv[2] & 0xF));
			val = _mm256_extract_epi64(mm256, 0);

		}

		mm256 = _mm256_set1_epi64x(val);
		mm256 = _mm256_rol_epi64(mm256, (iv[4] & 0xF));
		mm256 = _mm256_ror_epi64(mm256, (iv[1] & 0xF));

		return from_uint64(_mm256_extract_epi64(mm256, 0));
#endif
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
		for (size_t i{}; const auto& val : list) {
			if (i < Size) {
				data[i++] = encrypt(val);
			}
		}
	}
	explicit consteval obfuscxx(Type(&val)[Size])
	{
		for (size_t i{}; i < Size; ++i) {
			data[i] = encrypt(val[i]);
		}
	}

	FORCEINLINE Type get() const requires single
	{
		volatile const uint64_t* ptr = &data[0];
		uint64_t val = *ptr;
		return decrypt(val);
	}
	FORCEINLINE Type get(size_t i) const requires array
	{
		volatile const uint64_t* ptr = &data[i];
		uint64_t val = *ptr;
		return decrypt(val);
	}

	FORCEINLINE void copy_to(Type* out, size_t count) const requires array
	{
		size_t n = (count < Size) ? count : Size;
		for (size_t i = 0; i < n; ++i) {
			volatile const uint64_t* ptr = &data[i];
			out[i] = decrypt(*ptr);
		}
	}

	FORCEINLINE void set(Type val) requires single
	{
		data[0] = encrypt(val);
	}
	FORCEINLINE void set(Type val, size_t i) requires array
	{
		data[i] = encrypt(val);
	}
	FORCEINLINE void set(const std::initializer_list<Type>& list) requires array
	{
		for (size_t i{}; const auto& val : list) {
			if (i < Size) {
				data[i++] = encrypt(val);
			}
		}
	}

	FORCEINLINE Type operator()() const requires single
	{
		return decrypt(data[0]);
	}
	FORCEINLINE Type operator[](size_t i) const requires array
	{
		return decrypt(data[i]);
	}

	FORCEINLINE obfuscxx& operator=(Type val) requires single
	{
		set(val);
		return *this;
	}
	FORCEINLINE obfuscxx& operator=(const std::initializer_list<Type>& list) requires array
	{
		set(list);
		return *this;
	}

	FORCEINLINE bool operator==(const obfuscxx& rhs) const requires single
	{
		return get() == rhs.get();
	}
	FORCEINLINE bool operator==(const obfuscxx& rhs) const requires array
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

	FORCEINLINE operator Type() const requires single
	{
		return get();
	}

	FORCEINLINE bool operator<(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return get() < rhs.get();
	}
	FORCEINLINE bool operator>(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return get() > rhs.get();
	}
	FORCEINLINE bool operator<=(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return get() <= rhs.get();
	}
	FORCEINLINE bool operator>=(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return get() >= rhs.get();
	}

	FORCEINLINE obfuscxx operator+(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return obfuscxx(get() + rhs.get());
	}
	FORCEINLINE obfuscxx operator-(const obfuscxx& rhs) const requires (!single_pointer)
	{
		return obfuscxx(get() - rhs.get());
	}
	FORCEINLINE obfuscxx& operator+=(const obfuscxx& rhs) requires (!single_pointer)
	{
		set(get() + rhs.get());
		return *this;
	}
	FORCEINLINE obfuscxx& operator-=(const obfuscxx& rhs) requires (!single_pointer)
	{
		set(get() - rhs.get());
		return *this;
	}

	struct iterator {
		const obfuscxx* parent;
		size_t index;

		Type operator*() const { return parent->get(index); }
		iterator& operator++() { ++index; return *this; }
		bool operator!=(const iterator& other) const { return index != other.index; }
		bool operator==(const iterator& other) const { return index == other.index; }
	};

	iterator begin() const requires array { return { this, 0 }; }
	iterator end() const requires array { return { this, Size }; }
	static constexpr size_t size() { return Size; }

private:
	VOLATILE uint64_t data[Size]{};
};