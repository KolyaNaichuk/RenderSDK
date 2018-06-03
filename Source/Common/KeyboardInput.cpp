#include "Common/KeyboardInput.h"

BYTE KeyboardInput::m_KeyStates[NumKeys];

void KeyboardInput::Poll()
{
	VerifyWinAPIResult(GetKeyboardState(m_KeyStates));
}

bool KeyboardInput::IsKeyDown(Key key)
{
	return ((m_KeyStates[key] >> 7) != 0);
}

bool KeyboardInput::IsKeyUp(Key key)
{
	return ((m_KeyStates[key] >> 7) == 0);
}