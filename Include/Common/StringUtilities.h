#pragma once

#include "Common/Common.h"

bool AreEqual(const char* str1, const char* str2);

const std::wstring AnsiToWideString(const char* pAnsiString);
const std::string WideToAnsiString(const wchar_t* pWideString);