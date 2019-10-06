#include "Common/StringUtilities.h"

bool AreEqual(const char* str1, const char* str2)
{
	assert(str1 != nullptr);
	assert(str2 != nullptr);

	return (std::strcmp(str1, str2) == 0);
}

const std::wstring AnsiToWideString(const char* pAnsiString)
{
	const i32 BUFFER_SIZE = 512;
	wchar_t stringBuffer[BUFFER_SIZE];

	VerifyWinAPIResult(MultiByteToWideChar(CP_ACP, 0, pAnsiString, -1, stringBuffer, BUFFER_SIZE));
	return std::wstring(stringBuffer);
}

const std::string WideToAnsiString(const wchar_t* pWideString)
{
	const i32 BUFFER_SIZE = 512;
	char stringBuffer[BUFFER_SIZE];

	VerifyWinAPIResult(WideCharToMultiByte(CP_ACP, 0, pWideString, -1, stringBuffer, BUFFER_SIZE, NULL, NULL));
	return std::string(stringBuffer);
}
