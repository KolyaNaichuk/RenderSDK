#pragma once

#include <cstdint>

typedef std::int64_t		i64;
typedef std::int32_t		i32;
typedef std::int16_t		i16;
typedef std::int8_t			i8;

typedef std::uint64_t		u64;
typedef std::uint32_t		u32;
typedef std::uint16_t		u16;
typedef std::uint8_t		u8;

typedef float				f32;
typedef double				f64;

static_assert(sizeof(i8) == 1, "Type i8 is expected to be 1 byte");
static_assert(sizeof(u8) == 1, "Type u8 is expected to be 1 byte");
static_assert(sizeof(i16) == 2, "Type i16 is expected to be 2 bytes");
static_assert(sizeof(u16) == 2, "Type u16 is expected to be 2 bytes");
static_assert(sizeof(i32) == 4, "Type i32 is expected to be 4 bytes");
static_assert(sizeof(u32) == 4, "Type u32 is expected to be 4 bytes");
static_assert(sizeof(i64) == 8, "Type i64 is expected to be 8 bytes");
static_assert(sizeof(u64) == 8, "Type u64 is expected to be 8 bytes");
