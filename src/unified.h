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

#define internal    static
#define persist     static
#define global      static
#define aliasing    auto&
#define lambda      const auto

#define MACRO_CONCAT__(X, Y)                            X##Y
#define MACRO_CONCAT_(X, Y)                             MACRO_CONCAT__(X, Y)
#define EXPAND_(X)                                      X
#define OVERLOADED_MACRO_2_(_0, _1, MACRO, ...)         MACRO
#define OVERLOADED_MACRO_3_(_0, _1, _2, MACRO, ...)     MACRO
#define OVERLOADED_MACRO_4_(_0, _1, _2, _3, MACRO, ...) MACRO
#define FOR_INTERVAL_(NAME, MINI, MAXI)                 for (i32 NAME = (MINI); NAME < (MAXI); ++NAME)
#define FOR_INDICIES_(NAME, MAXI)                       FOR_INTERVAL_(NAME, 0, (MAXI))
#define FOR_REPEAT_(MAXI)                               FOR_INTERVAL_(MACRO_CONCAT_(FOR_REPEAT_, __LINE__), 0, (MAXI))
#define FOR_INTERVAL_REV_(NAME, MINI, MAXI)             for (i32 NAME = (MAXI) - 1, MACRO_CONCAT_(NAME, min); NAME >= MACRO_CONCAT_(NAME, min); --NAME)
#define FOR_INDICIES_REV_(NAME, MAXI)                   FOR_INTERVAL_REV_(NAME, 0, (MAXI))
#define CAPACITY_OF_ARRAY_(XS)                          (sizeof(XS) / sizeof((XS)[0]))
#define CAPACITY_OF_MEMBER_(TYPE, MEMBER)               (CAPACITY_OF_ARRAY_(((TYPE*) 0)->MEMBER))
#define FOR_POINTER_(NAME, XS, COUNT)                   for (i32 MACRO_CONCAT_(NAME, _index) = 0; MACRO_CONCAT_(NAME, _index) < (COUNT); ++MACRO_CONCAT_(NAME, _index)) if (const auto NAME = &(XS)[MACRO_CONCAT_(NAME, _index)]; false); else
#define FOR_ARRAY_(NAME, XS)                            FOR_POINTER_(NAME, (XS), ARRAY_CAPACITY(XS))
#define FOR_POINTER_REV_(NAME, XS, COUNT)               for (i32 MACRO_CONCAT_(NAME, _index) = (COUNT) - 1; MACRO_CONCAT_(NAME, _index) >= 0; --MACRO_CONCAT_(NAME, _index)) if (const auto NAME = &(XS)[MACRO_CONCAT_(NAME, _index)]; false); else
#define FOR_ARRAY_REV_(NAME, XS)                        FOR_POINTER_REV_(NAME, (XS), ARRAY_CAPACITY(XS))

#define ARRAY_CAPACITY(...)         (EXPAND_(OVERLOADED_MACRO_2_(__VA_ARGS__, CAPACITY_OF_MEMBER_, CAPACITY_OF_ARRAY_)(__VA_ARGS__)))
#define FOR_RANGE(...)              EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_INTERVAL_, FOR_INDICIES_, FOR_REPEAT_)(__VA_ARGS__))
#define FOR_RANGE_REV(...)          EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_INTERVAL_REV_, FOR_INDICIES_REV_)(__VA_ARGS__))
#define FOR_ELEMS(...)              EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_POINTER_, FOR_ARRAY_)(__VA_ARGS__))
#define FOR_ELEMS_REV(...)          EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_POINTER_REV_, FOR_ARRAY_REV_)(__VA_ARGS__))
#define IN_RANGE(X, MINI, MAXI)     ((MINI) <= (X) && (X) < (MAXI))
#define SWAP(X, Y)                  do { auto MACRO_CONCAT_(TEMP_SWAP_, __LINE__) = *(X); *(X) = *(Y); *(Y) = MACRO_CONCAT_(TEMP_SWAP_, __LINE__); } while (false)
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

#define enum_start_region(NAME) MACRO_CONCAT_(NAME, _START), MACRO_CONCAT_(NAME, _START_) = MACRO_CONCAT_(NAME, _START) - 1,
#define enum_end_region(NAME)   MACRO_CONCAT_(NAME, _END), MACRO_CONCAT_(NAME, _COUNT) = MACRO_CONCAT_(NAME, _END) - MACRO_CONCAT_(NAME, _START), MACRO_CONCAT_(NAME, _END_) = MACRO_CONCAT_(NAME, _END) - 1,

#if DEBUG
#undef MOUSE_MOVED
#include <windows.h>
#undef interface
#include <stdio.h>
#define DEBUG_printf(FSTR, ...) do { char MACRO_CONCAT_(TEMP_DEBUG_PRINTF_, __LINE__)[512]; sprintf_s(MACRO_CONCAT_(TEMP_DEBUG_PRINTF_, __LINE__), sizeof(MACRO_CONCAT_(TEMP_DEBUG_PRINTF_, __LINE__)), (FSTR), __VA_ARGS__); OutputDebugStringA(MACRO_CONCAT_(TEMP_DEBUG_PRINTF_, __LINE__)); } while (false)
#define DEBUG_once              for (persist bool32 MACRO_CONCAT_(DEBUG_ONCE_, __LINE__) = true; MACRO_CONCAT_(DEBUG_ONCE_, __LINE__); MACRO_CONCAT_(DEBUG_ONCE_, __LINE__) = false)

#define NUM_ARGS(...) _NUM_ARGS(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define EXPAND(X)       X
#define FIRSTARG(X, ...)    (X)
#define RESTARGS(X, ...)    (__VA_ARGS__)
#define FOREACH(MACRO, LIST)    FOREACH_(NUM_ARGS LIST, MACRO, LIST)
#define FOREACH_(N, M, LIST)    FOREACH__(N, M, LIST)
#define FOREACH__(N, M, LIST)   FOREACH_##N(M, LIST)
#define FOREACH_1(M, LIST)  M LIST
#define FOREACH_2(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_1(M, RESTARGS LIST)
#define FOREACH_3(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_2(M, RESTARGS LIST)
#define FOREACH_4(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_3(M, RESTARGS LIST)
#define FOREACH_5(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_4(M, RESTARGS LIST)
#define FOREACH_6(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_5(M, RESTARGS LIST)
#define FOREACH_7(M, LIST)  EXPAND(M FIRSTARG LIST) FOREACH_6(M, RESTARGS LIST)
#define MY_VARIADIC_MACRO(...)    FOREACH(STRINGIFY, (__VA_ARGS__))

#define _GET_NTH_ARG(_1, _2, _3, _4, N, ...) N
#define _fe_0(_call, ...)
#define _fe_1(_call, x) _call(x)
#define _fe_2(_call, x, ...) _call(x) _fe_1(_call, __VA_ARGS__)
#define _fe_3(_call, x, ...) _call(x) EXPAND_(_fe_2(_call, __VA_ARGS__))
#define _fe_4(_call, x, ...) _call(x) EXPAND_(_fe_3(_call, __VA_ARGS__))

#define CALL_MACRO_X_FOR_EACH(x, ...) \
EXPAND(_GET_NTH_ARG(__VA_ARGS__, _fe_4, _fe_3, _fe_2, _fe_1, _fe_0)(x, __VA_ARGS__))

#define STRINGIFY_COMMA_(X) #X,
#define MACRO_ARGS_STRINGIFY_(...) CALL_MACRO_X_FOR_EACH(STRINGIFY_COMMA_, __VA_ARGS__)

#define DEBUG_PROFILER_create_group(GROUP_NAME, ...)\
enum struct GROUP_NAME : u8 { __VA_ARGS__, CAPACITY };\
constexpr strlit        MACRO_CONCAT_(GROUP_NAME, _STRS)[GROUP_NAME::CAPACITY] = { MACRO_ARGS_STRINGIFY_(__VA_ARGS__) };\
persist   LARGE_INTEGER MACRO_CONCAT_(GROUP_NAME, _LI_0)[GROUP_NAME::CAPACITY] = {};\
persist   LARGE_INTEGER MACRO_CONCAT_(GROUP_NAME, _LI_1)[GROUP_NAME::CAPACITY] = {};\
persist   u64           MACRO_CONCAT_(GROUP_NAME, _DATA)[GROUP_NAME::CAPACITY] = {}

#define DEBUG_PROFILER_flush_group(GROUP_NAME, COUNT)\
persist u64 MACRO_CONCAT_(GROUP_NAME, _COUNTER) = 0;\
MACRO_CONCAT_(GROUP_NAME, _COUNTER) += 1;\
if (MACRO_CONCAT_(GROUP_NAME, _COUNTER) >= (COUNT))\
{\
	MACRO_CONCAT_(GROUP_NAME, _COUNTER) = 0;\
	u64 MACRO_CONCAT_(GROUP_NAME, _TOTAL) = 0;\
	FOR_RANGE(MACRO_CONCAT_(GROUP_NAME, _INDEX), static_cast<u8>(GROUP_NAME::CAPACITY))\
	{\
		MACRO_CONCAT_(GROUP_NAME, _TOTAL) += MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)];\
	}\
	DEBUG_printf(#GROUP_NAME " TOTAL\n\t%llu\n", MACRO_CONCAT_(GROUP_NAME, _TOTAL));\
	FOR_RANGE(MACRO_CONCAT_(GROUP_NAME, _INDEX), static_cast<u8>(GROUP_NAME::CAPACITY))\
	{\
		DEBUG_printf("%s\n\t(%.3f) %llu\n", MACRO_CONCAT_(GROUP_NAME, _STRS)[MACRO_CONCAT_(GROUP_NAME, _INDEX)], MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)] / static_cast<f64>(MACRO_CONCAT_(GROUP_NAME, _TOTAL)), MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)]);\
		MACRO_CONCAT_(GROUP_NAME, _DATA)[MACRO_CONCAT_(GROUP_NAME, _INDEX)] = 0;\
	}\
	DEBUG_printf("\n");\
}

#define DEBUG_PROFILER_start(GROUP_NAME, SUBGROUP_NAME)\
do { QueryPerformanceCounter(&MACRO_CONCAT_(GROUP_NAME, _LI_0)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)]); } while (false)

#define DEBUG_PROFILER_end(GROUP_NAME, SUBGROUP_NAME)\
do\
{\
	QueryPerformanceCounter(&MACRO_CONCAT_(GROUP_NAME, _LI_1)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)]);\
	MACRO_CONCAT_(GROUP_NAME, _DATA)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)] += MACRO_CONCAT_(GROUP_NAME, _LI_1)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)].QuadPart - MACRO_CONCAT_(GROUP_NAME, _LI_0)[static_cast<u8>(GROUP_NAME::SUBGROUP_NAME)].QuadPart;\
} while (false)
#else
#define DEBUG_printf(FSTR, ...)
#define DEBUG_once                                                  if (true); else
#define DEBUG_PROFILER_create_group(...)
#define DEBUG_PROFILER_flush_group(...)
#define DEBUG_PROFILER_start(...)
#define DEBUG_PROFILER_end(...)
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

#define memory_arena_checkpoint(ARENA) memsize MACRO_CONCAT_(MEMORY_ARENA_CHECKPOINT, __LINE__) = (ARENA)->used; DEFER { (ARENA)->used = MACRO_CONCAT_(MEMORY_ARENA_CHECKPOINT, __LINE__); }

struct MemoryArena
{
	memsize size;
	byte*   base;
	memsize used;
};

template <typename TYPE>
internal TYPE* memory_arena_allocate(MemoryArena* arena, memsize count = 1)
{
	ASSERT(arena->used + sizeof(TYPE) * count <= arena->size);
	byte* allocation = arena->base + arena->used;
	arena->used += sizeof(TYPE) * count;
	return reinterpret_cast<TYPE*>(allocation);
}

template <typename TYPE>
internal TYPE* memory_arena_allocate_zero(MemoryArena* arena, memsize count = 1)
{
	ASSERT(arena->used + sizeof(TYPE) * count <= arena->size);
	byte* allocation = arena->base + arena->used;
	memset(allocation, static_cast<unsigned char>(0), sizeof(TYPE) * count);
	arena->used += sizeof(TYPE) * count;
	return reinterpret_cast<TYPE*>(allocation);
}

//
// Math.
//

template <typename TYPE>
internal constexpr TYPE clamp(TYPE x, TYPE a, TYPE b)
{
	return x <= a ? a : x >= b ? b : x;
}

struct vf2
{
	f32 x; f32 y;
};

union vf3
{
	struct { f32 x; f32 y; f32 z; };
	vf2 xy;
};

union vf4
{
	struct { f32 x; f32 y; f32 z; f32 w; };
	vf3 xyz;
	vf2 xy;
};

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

struct vi2
{
	i32 x; i32 y;
};

union vi3
{
	struct { i32 x; i32 y; i32 z; };
	vi2 xy;
};

union vi4
{
	struct { i32 x; i32 y; i32 z; i32 w; };
	vi3 xyz;
	vi2 xy;
};

internal constexpr bool32 operator+ (vi2  v       ) { return v.x || v.y;               }
internal constexpr bool32 operator+ (vi3  v       ) { return v.x || v.y || v.z;        }
internal constexpr bool32 operator+ (vi4  v       ) { return v.x || v.y || v.z || v.w; }
internal constexpr vi2    operator- (vi2  v       ) { return { -v.x, -v.y             }; }
internal constexpr vi3    operator- (vi3  v       ) { return { -v.x, -v.y, -v.z       }; }
internal constexpr vi4    operator- (vi4  v       ) { return { -v.x, -v.y, -v.z, -v.w }; }
internal constexpr bool32 operator==(vi2  u, vi2 v) { return u.x == v.x && u.y == v.y;                             }
internal constexpr bool32 operator==(vi3  u, vi3 v) { return u.x == v.x && u.y == v.y && u.z == v.z;               }
internal constexpr bool32 operator==(vi4  u, vi4 v) { return u.x == v.x && u.y == v.y && u.z == v.z && u.w == v.w; }
internal constexpr bool32 operator!=(vi2  u, vi2 v) { return !(u == v); }
internal constexpr bool32 operator!=(vi3  u, vi3 v) { return !(u == v); }
internal constexpr bool32 operator!=(vi4  u, vi4 v) { return !(u == v); }
internal constexpr vi2    operator+ (vi2  u, vi2 v) { return { u.x + v.x, u.y + v.y                       }; }
internal constexpr vi3    operator+ (vi3  u, vi3 v) { return { u.x + v.x, u.y + v.y, u.z + v.z            }; }
internal constexpr vi4    operator+ (vi4  u, vi4 v) { return { u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w }; }
internal constexpr vi2    operator- (vi2  u, vi2 v) { return { u.x - v.x, u.y - v.y                       }; }
internal constexpr vi3    operator- (vi3  u, vi3 v) { return { u.x - v.x, u.y - v.y, u.z - v.z            }; }
internal constexpr vi4    operator- (vi4  u, vi4 v) { return { u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w }; }
internal constexpr vi2    operator/ (vi2  v, i32 k) { return { v.x / k, v.y / k                   }; }
internal constexpr vi3    operator/ (vi3  v, i32 k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vi4    operator/ (vi4  v, i32 k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vi2    operator* (vi2  v, i32 k) { return { v.x * k, v.y * k                   }; }
internal constexpr vi3    operator* (vi3  v, i32 k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vi4    operator* (vi4  v, i32 k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vi2    operator* (i32  k, vi2 v) { return v * k; }
internal constexpr vi3    operator* (i32  k, vi3 v) { return v * k; }
internal constexpr vi4    operator* (i32  k, vi4 v) { return v * k; }
internal constexpr vi2&   operator+=(vi2& u, vi2 v) { return u = u + v; }
internal constexpr vi3&   operator+=(vi3& u, vi3 v) { return u = u + v; }
internal constexpr vi4&   operator+=(vi4& u, vi4 v) { return u = u + v; }
internal constexpr vi2&   operator-=(vi2& u, vi2 v) { return u = u - v; }
internal constexpr vi3&   operator-=(vi3& u, vi3 v) { return u = u - v; }
internal constexpr vi4&   operator-=(vi4& u, vi4 v) { return u = u - v; }
internal constexpr vi2&   operator*=(vi2& v, i32 k) { return v = v * k; }
internal constexpr vi3&   operator*=(vi3& v, i32 k) { return v = v * k; }
internal constexpr vi4&   operator*=(vi4& v, i32 k) { return v = v * k; }
internal constexpr vi2&   operator/=(vi2& v, i32 k) { return v = v / k; }
internal constexpr vi3&   operator/=(vi3& v, i32 k) { return v = v / k; }
internal constexpr vi4&   operator/=(vi4& v, i32 k) { return v = v / k; }

internal constexpr vf2    vx2(f32 a)        { return {   a,   a         }; }
internal constexpr vi2    vx2(i32 a)        { return {   a,   a         }; }
internal constexpr vf3    vx3(f32 a)        { return {   a,   a,   a    }; }
internal constexpr vf3    vx3(vf2 v, f32 a) { return { v.x, v.y,      a }; }
internal constexpr vf4    vx4(vf3 v, f32 a) { return { v.x, v.y, v.z, a }; }

internal constexpr vf3    vxx(vf2 v, f32 a) { return { v.x, v.y, a      }; }
internal constexpr vf4    vxx(vf3 v, f32 a) { return { v.x, v.y, v.z, a }; }

internal constexpr vf2    vxx(vi2 v) { return { static_cast<f32>(v.x), static_cast<f32>(v.y)                                               }; }
internal constexpr vi2    vxx(vf2 v) { return { static_cast<i32>(v.x), static_cast<i32>(v.y)                                               }; }
internal constexpr vf3    vxx(vi3 v) { return { static_cast<f32>(v.x), static_cast<f32>(v.y), static_cast<f32>(v.z)                        }; }
internal constexpr vi3    vxx(vf3 v) { return { static_cast<i32>(v.x), static_cast<i32>(v.y), static_cast<i32>(v.z)                        }; }
internal constexpr vf4    vxx(vi4 v) { return { static_cast<f32>(v.x), static_cast<f32>(v.y), static_cast<f32>(v.z), static_cast<f32>(v.w) }; }
internal constexpr vi4    vxx(vf4 v) { return { static_cast<i32>(v.x), static_cast<i32>(v.y), static_cast<i32>(v.z), static_cast<i32>(v.w) }; }

internal constexpr vf2    operator+ (vf2  u, vi2 v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y)                                                           }; }
internal constexpr vf3    operator+ (vf3  u, vi3 v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y), u.z + static_cast<f32>(v.z)                              }; }
internal constexpr vf4    operator+ (vf4  u, vi4 v) { return { u.x + static_cast<f32>(v.x), u.y + static_cast<f32>(v.y), u.z + static_cast<f32>(v.z), u.w + static_cast<f32>(v.w) }; }
internal constexpr vf2    operator+ (vi2  u, vf2 v) { return v + u; }
internal constexpr vf3    operator+ (vi3  u, vf3 v) { return v + u; }
internal constexpr vf4    operator+ (vi4  u, vf4 v) { return v + u; }
internal constexpr vf2    operator- (vf2  u, vi2 v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y)                                                           }; }
internal constexpr vf3    operator- (vf3  u, vi3 v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y), u.z - static_cast<f32>(v.z)                              }; }
internal constexpr vf4    operator- (vf4  u, vi4 v) { return { u.x - static_cast<f32>(v.x), u.y - static_cast<f32>(v.y), u.z - static_cast<f32>(v.z), u.w - static_cast<f32>(v.w) }; }
internal constexpr vf2    operator- (vi2  u, vf2 v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y                                                           }; }
internal constexpr vf3    operator- (vi3  u, vf3 v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y, static_cast<f32>(u.z) - v.z                              }; }
internal constexpr vf4    operator- (vi4  u, vf4 v) { return { static_cast<f32>(u.x) - v.x, static_cast<f32>(u.y) - v.y, static_cast<f32>(u.z) - v.z, static_cast<f32>(u.w) - v.w }; }
internal constexpr vf2    operator/ (vi2  v, f32 k) { return { v.x / k, v.y / k                   }; }
internal constexpr vf3    operator/ (vi3  v, f32 k) { return { v.x / k, v.y / k, v.z / k          }; }
internal constexpr vf4    operator/ (vi4  v, f32 k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal constexpr vf2    operator* (vi2  v, f32 k) { return { v.x * k, v.y * k                   }; }
internal constexpr vf3    operator* (vi3  v, f32 k) { return { v.x * k, v.y * k, v.z * k          }; }
internal constexpr vf4    operator* (vi4  v, f32 k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal constexpr vf2    operator* (f32  k, vi2 v) { return v * k; }
internal constexpr vf3    operator* (f32  k, vi3 v) { return v * k; }
internal constexpr vf4    operator* (f32  k, vi4 v) { return v * k; }
