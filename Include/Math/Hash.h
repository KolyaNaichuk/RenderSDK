#pragma once

#include "Common/Common.h"

template <typename T>
void HashCombine(std::size_t& seed, const T& value)
{
	seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
