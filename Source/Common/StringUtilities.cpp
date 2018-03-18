#include "Common/StringUtilities.h"

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
