#include "Common/StringUtilities.h"

const std::wstring AnsiToWideString(const char* pAnsiString)
{
	int ansiStringLength = std::strlen(pAnsiString);
	assert(ansiStringLength > 0);

	int wideStringLength = MultiByteToWideChar(CP_ACP, 0, pAnsiString, ansiStringLength, nullptr, 0);
	std::wstring wideString(wideStringLength, L'\0');
	int numWrittenCharacters = MultiByteToWideChar(CP_ACP, 0, pAnsiString, ansiStringLength, &wideString[0], wideStringLength);
	assert(numWrittenCharacters == wideStringLength);

	return wideString;
}