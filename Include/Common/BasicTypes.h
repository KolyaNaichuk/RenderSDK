#pragma once

typedef signed __int64		i64;
typedef signed int			i32;
typedef signed short		i16;
typedef signed char			i8;

typedef unsigned __int64	u64;
typedef unsigned int		u32;
typedef unsigned short		u16;
typedef unsigned char		u8;

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
