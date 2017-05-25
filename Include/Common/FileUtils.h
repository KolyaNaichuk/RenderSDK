#pragma once

#include "Common/Common.h"

enum class FileMode
{
	Text,
	Binary
};

bool LoadDataFromFile(const wchar_t* pFilePath, FileMode fileMode, std::vector<char>& loadedData);
i64 GetFileSizeInBytes(HANDLE hFile);