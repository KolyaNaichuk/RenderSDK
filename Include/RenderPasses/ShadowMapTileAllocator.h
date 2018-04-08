#pragma once

#include "Math/Vector2.h"

struct ShadowMapTile
{
	Vector2f m_TexSpaceTopLeft;
	f32 m_TexSpaceSize = 0.0f;
};

class ShadowMapTileAllocator
{
public:
	ShadowMapTileAllocator(u32 shadowMapSize, u32 minTileSize);
	~ShadowMapTileAllocator();

	bool Allocate(u32 tileSize, ShadowMapTile* pNewTile);
	void Reset();

private:
	u32 CalcTreeLevel(u32 tileSize) const;
	u32 FindFreeNode(u32 currentLevel, u32 currentIndex, u32 requiredLevel);

private:
	u32 m_MaxTileSize = 0;
	f32 m_RcpMaxTileSize = 0.0f;
	f32 m_Log2MaxTileSize = 0.0f;
	u32 m_MinTileSize = 0;
	u32 m_NumNodes = 0;
	u8* m_pFreeNodes = nullptr;
	ShadowMapTile* m_pTiles = nullptr;
};