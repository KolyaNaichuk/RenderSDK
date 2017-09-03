#pragma once

#include "Common/Common.h"

enum class FileMode
{
	Text,
	Binary
};

bool LoadDataFromFile(const wchar_t* pFilePath, FileMode fileMode, std::vector<char>& loadedByteData);
i64 GetFileSizeInBytes(HANDLE hFile);
const std::wstring ExtractFileExtension(const std::wstring& filePath);
const std::wstring ExtractFileNameWithExtension(const std::wstring& filePath);