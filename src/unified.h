#pragma once
#include <stdint.h>
#include <utility>

#if DEBUG
#include <assert.h>
#define ASSERT(EXPRESSION)        do { if (!(EXPRESSION)) { *((int*)(0)) = 0; } } while (false)
#define STATIC_ASSERT(EXPRESSION) static_assert(EXPRESSION)
#else
#define ASSERT(EXPRESSION)
#define STATIC_ASSERT(EXPRESSION)
#endif

#define internal static
#define persist  static
#define global   static
#define refering const auto&

#define MACRO_CONCAT__(X, Y)                            X##Y
#define MACRO_CONCAT_(X, Y)                             MACRO_CONCAT__(X, Y)
#define EXPAND_(X)                                      X
#define OVERLOADED_MACRO_2_(_0, _1, MACRO, ...)         MACRO
#define OVERLOADED_MACRO_3_(_0, _1, _2, MACRO, ...)     MACRO
#define OVERLOADED_MACRO_4_(_0, _1, _2, _3, MACRO, ...) MACRO
#define FOR_INTERVAL_(NAME, MINI, MAXI)                 for (i32 NAME = (MINI); NAME < (MAXI); ++NAME)
#define FOR_INDICIES_(NAME, MAXI)                       FOR_INTERVAL_(NAME, 0, (MAXI))
#define FOR_REPEAT_(MAXI)                               FOR_INTERVAL_(MACRO_CONCAT_(FOR_REPEAT_, __LINE__), 0, (MAXI))
#define FOR_INTERVAL_REV_(NAME, MINI, MAXI)             for (i32 NAME = (MAXI) - 1; NAME >= (MINI); --NAME)
#define FOR_INDICIES_REV_(NAME, MAXI)                   FOR_INTERVAL_REV_(NAME, 0, (MAXI))
#define CAPACITY_OF_ARRAY_(XS)                          (sizeof(XS) / sizeof((XS)[0]))
#define CAPACITY_OF_MEMBER_(TYPE, MEMBER)               (CAPACITY_OF_ARRAY_(((TYPE*) 0)->MEMBER))
#define FOR_POINTER_(NAME, XS, COUNT)                   for (i32 NAME##_index = 0; NAME##_index < (COUNT); ++NAME##_index) if (const auto NAME = &(XS)[NAME##_index]; false); else
#define FOR_ARRAY_(NAME, XS)                            FOR_POINTER_(NAME, (XS), ARRAY_CAPACITY(XS))
#define FOR_POINTER_REV_(NAME, XS, COUNT)               for (i32 NAME##_index = (COUNT) - 1; NAME##_index >= 0; --NAME##_index) if (const auto NAME = &(XS)[NAME##_index]; false); else
#define FOR_ARRAY_REV_(NAME, XS)                        FOR_POINTER_REV_(NAME, (XS), ARRAY_CAPACITY(XS))

#define ARRAY_CAPACITY(...)         (EXPAND_(OVERLOADED_MACRO_2_(__VA_ARGS__, CAPACITY_OF_MEMBER_, CAPACITY_OF_ARRAY_)(__VA_ARGS__)))
#define FOR_RANGE(...)              EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_INTERVAL_, FOR_INDICIES_, FOR_REPEAT_)(__VA_ARGS__))
#define FOR_RANGE_REV(...)          EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_INTERVAL_REV_, FOR_INDICIES_REV_)(__VA_ARGS__))
#define FOR_ELEMS(...)              EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_POINTER_, FOR_ARRAY_)(__VA_ARGS__))
#define FOR_ELEMS_REV(...)          EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_POINTER_REV_, FOR_ARRAY_REV_)(__VA_ARGS__))
#define IN_RANGE(X, MINI, MAXI)     ((MINI) <= (X) && (X) < (MAXI))
#define MINIMUM(X, Y)               ((X) <= (Y) ? (X) : (Y))
#define MAXIMUM(X, Y)               ((X) >= (Y) ? (X) : (Y))
#define CLAMP(X, MINI, MAXI)        ((X) < (MINI) ? (MINI) : (X) > (MAXI) ? (MAXI) : (X))
#define SWAP(X, Y)                  do { auto TEMP_SWAP_##__LINE__ = X; X = Y; Y = TEMP_SWAP_##__LINE__; } while (false)
#define KIBIBYTES_OF(N)             (1024LL *             (N))
#define MEBIBYTES_OF(N)             (1024LL * KIBIBYTES_OF(N))
#define GIBIBYTES_OF(N)             (1024LL * MEBIBYTES_OF(N))
#define TEBIBYTES_OF(N)             (1024LL * GIBIBYTES_OF(N))

#define enum_loose(NAME, TYPE)\
enum struct NAME : TYPE;\
internal constexpr TYPE operator+  (NAME a        ) { return static_cast<TYPE>(a); }\
internal constexpr TYPE operator<  (NAME a, NAME b) { return +a <  +b; }\
internal constexpr TYPE operator<= (NAME a, NAME b) { return +a <= +b; }\
internal constexpr TYPE operator>  (NAME a, NAME b) { return +a >  +b; }\
internal constexpr TYPE operator>= (NAME a, NAME b) { return +a >= +b; }\
enum struct NAME : TYPE

#define flag_struct(NAME, TYPE)\
enum struct NAME : TYPE;\
internal constexpr TYPE operator+  (NAME  a        ) { return     static_cast<TYPE>(a);            }\
internal constexpr NAME operator&  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) & (+b)); }\
internal constexpr NAME operator|  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) | (+b)); }\
internal constexpr NAME operator^  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) ^ (+b)); }\
internal constexpr NAME operator<< (NAME  a, i32  n) { return     static_cast<NAME>( (+a) << n  ); }\
internal constexpr NAME operator>> (NAME  a, i32  n) { return     static_cast<NAME>( (+a) >> n  ); }\
internal constexpr NAME operator~  (NAME  a        ) { return     static_cast<NAME>( ~+a        ); }\
internal constexpr NAME operator&= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) & (+b)); }\
internal constexpr NAME operator|= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) | (+b)); }\
internal constexpr NAME operator^= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) ^ (+b)); }\
internal constexpr NAME operator<<=(NAME& a, i32  n) { return a = static_cast<NAME>( (+a) << n  ); }\
internal constexpr NAME operator>>=(NAME& a, i32  n) { return a = static_cast<NAME>( (+a) >> n  ); }\
enum struct NAME : TYPE

#if DEBUG
#undef MOUSE_MOVED
#include <windows.h>
#undef interface
#include <stdio.h>
#define DEBUG_printf(FSTR, ...) do { char TEMP_DEBUG_PRINTF_##__LINE__[512]; sprintf_s(TEMP_DEBUG_PRINTF_##__LINE__, sizeof(TEMP_DEBUG_PRINTF_##__LINE__), (FSTR), __VA_ARGS__); OutputDebugStringA(TEMP_DEBUG_PRINTF_##__LINE__); } while (false)
#define DEBUG_once              for (persist bool32 DEBUG_ONCE_##__LINE__ = true; DEBUG_ONCE_##__LINE__; DEBUG_ONCE_##__LINE__ = false)
#else
#define DEBUG_printf(FSTR, ...)
#define DEBUG_once
#endif

#define DEFER auto MACRO_CONCAT_(DEFER_, __LINE__) = DEFER_EMPTY_ {} + [&]()

template <typename F>
struct DEFER_
{
	F f;
	DEFER_(F f) : f(f) {}
	~DEFER_() { f(); }
};

struct DEFER_EMPTY_ {};

template <typename F>
internal DEFER_<F> operator+(DEFER_EMPTY_, F&& f)
{
	return DEFER_<F>(std::forward<F>(f));
}

typedef uint8_t     byte;
typedef uint64_t    memsize;
typedef const char* strlit;
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef uint8_t     bool8;
typedef uint16_t    bool16;
typedef uint32_t    bool32;
typedef uint64_t    bool64;
typedef float       f32;
typedef double      f64;

//
// Memory.
//

struct MemoryArena
{
	memsize size;
	byte*   base;
	memsize used;
};

template <typename TYPE>
internal TYPE* memory_arena_push(MemoryArena* arena, i32 count = 1)
{
	ASSERT(arena->used + sizeof(TYPE) * count <= arena->size);
	byte* allocation = arena->base + arena->used;
	arena->used += sizeof(TYPE) * count;
	return reinterpret_cast<TYPE*>(allocation);
}

//
// Math.
//

struct vf2 { f32 x; f32 y;               };
struct vf3 { f32 x; f32 y; f32 z;        };
struct vf4 { f32 x; f32 y; f32 z; f32 w; };

internal constexpr bool32 operator+ (vf2  v       ) { return v.x || v.y;               }
internal constexpr bool32 operator+ (vf3  v       ) { return v.x || v.y || v.z;        }
internal constexpr bool32 operator+ (vf4  v       ) { return v.x || v.y || v.z || v.w; }
internal constexpr vf2    operator- (vf2  v       ) { return { -v.x, -v.y             }; }
internal constexpr vf3    operator- (vf3  v       ) { return { -v.x, -v.y, -v.z       }; }
internal constexpr vf4    operator- (vf4  v       ) { return { -v.x, -v.y, -v.z, -v.w }; }
internal constexpr bool32 operator==(vf2  u, vf2 v) { return u.x == v.x && u.y == v.y;                             }
internal constexpr bool32 operator==(vf3  u, vf3 v) { return u.x == v.x && u.y == v.y && u.z == v.z;               }
internal constexpr bool32 operator==(vf4  u, vf4 v) { return u.x == v.x && u.y == v.y && u.z == v.z && u.w == v.w; }
internal constexpr bool32 operator!=(vf2  u, vf2 v) { return !(u == v); }
internal constexpr bool32 operator!=(vf3  u, vf3 v) { return !(u == v); }
internal constexpr bool32 operator!=(vf4  u, vf4 v) { return !(u == v); }
internal constexpr vf2    operator+ (vf2  u, vf2 v) { return { u.x + v.x, u.y + v.y                       }; }
internal constexpr vf3    operator+ (vf3  u, vf3 v) { return { u.x + v.x, u.y + v.y, u.z + v.z            }; }
internal constexpr vf4    operator+ (vf4  u, vf4 v) { return { u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w }; }
internal constexpr vf2    operator- (vf2  u, vf2 v) { return { u.x - v.x, u.y - v.y                       }; }
internal constexpr vf3    operator- (vf3  u, vf3 v) { return { u.x - v.x, u.y - v.y, u.z - v.z            }; }
internal constexpr vf4    operator- (vf4  u, vf4 v) { return { u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w }; }
internal constexpr vf2    operator/ (vf2  v, f32 k) { return { v.x / k, v.y / k                   }; }
internal constexpr vf3    operator/ (vf3  v, f32 k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vf4    operator/ (vf4  v, f32 k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vf2    operator* (vf2  v, f32 k) { return { v.x * k, v.y * k                   }; }
internal constexpr vf3    operator* (vf3  v, f32 k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vf4    operator* (vf4  v, f32 k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vf2    operator* (f32  k, vf2 v) { return v * k; }
internal constexpr vf3    operator* (f32  k, vf3 v) { return v * k; }
internal constexpr vf4    operator* (f32  k, vf4 v) { return v * k; }
internal constexpr vf2&   operator+=(vf2& u, vf2 v) { return u = u + v; }
internal constexpr vf3&   operator+=(vf3& u, vf3 v) { return u = u + v; }
internal constexpr vf4&   operator+=(vf4& u, vf4 v) { return u = u + v; }
internal constexpr vf2&   operator-=(vf2& u, vf2 v) { return u = u - v; }
internal constexpr vf3&   operator-=(vf3& u, vf3 v) { return u = u - v; }
internal constexpr vf4&   operator-=(vf4& u, vf4 v) { return u = u - v; }
internal constexpr vf2&   operator*=(vf2& v, f32 k) { return v = v * k; }
internal constexpr vf3&   operator*=(vf3& v, f32 k) { return v = v * k; }
internal constexpr vf4&   operator*=(vf4& v, f32 k) { return v = v * k; }
internal constexpr vf2&   operator/=(vf2& v, f32 k) { return v = v / k; }
internal constexpr vf3&   operator/=(vf3& v, f32 k) { return v = v / k; }
internal constexpr vf4&   operator/=(vf4& v, f32 k) { return v = v / k; }
