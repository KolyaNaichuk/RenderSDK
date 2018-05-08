#pragma once

#include "D3DWrapper/CommandSignature.h"

struct DrawCommand
{
	UINT m_InstanceOffset;
	UINT m_MaterialIndex;
	DrawIndexedArguments m_Args;
};

struct ShadowMapCommand
{
	UINT m_DataOffset;
	DrawIndexedArguments m_Args;
};
