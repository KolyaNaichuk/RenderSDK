#pragma once

#include "Common/Common.h"

class KeyboardInput
{
public:
	enum Key
	{
		Key_W = 0x57,
		Key_S = 0x53,
		Key_A = 0x41,
		Key_D = 0x44,
		Key_E = 0x45,
		Key_Q = 0x51,
		Key_Up = 0x26,
		Key_Down = 0x28,
		Key_Left = 0x25,
		Key_Right = 0x27,
		Key_0 = 0x30,
		Key_1 = 0x31,
		Key_2 = 0x32,
		Key_3 = 0x33,
		Key_4 = 0x34,
		Key_5 = 0x35,
		Key_6 = 0x36,
		Key_7 = 0x37,
		Key_8 = 0x38,
		Key_9 = 0x39
	};

	static void Poll();

	static bool IsKeyDown(Key key);
	static bool IsKeyUp(Key key);

private:
	enum { NumKeys = 256 };
	static BYTE m_KeyStates[NumKeys];
};