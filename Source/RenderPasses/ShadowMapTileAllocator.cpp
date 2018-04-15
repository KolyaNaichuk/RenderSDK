#include "RenderPasses/ShadowMapTileAllocator.h"

namespace
{
	const u32 INVALID_INDEX = ~0u;
}

ShadowMapTileAllocator::ShadowMapTileAllocator(u32 shadowMapSize, u32 minTileSize)
{
	assert(IsPowerOf2(shadowMapSize));
	assert(IsPowerOf2(minTileSize));

	const f32 shadowMapSizeF = f32(shadowMapSize);

	m_MaxTileSize = shadowMapSize;
	m_RcpMaxTileSize = Rcp(shadowMapSizeF);
	m_Log2MaxTileSize = Log2(shadowMapSizeF);
	m_MinTileSize = minTileSize;

	const u32 numLevels = 1 + CalcTreeLevel(minTileSize);
	m_NumNodes = ((2 << ((numLevels << 1) - 1)) - 1) / 3;

	m_pFreeNodes = new u8[m_NumNodes];
	m_pTiles = new ShadowMapTile[m_NumNodes];

	struct TileNode
	{
		TileNode(const Vector2i& center, i32 size)
			: m_Center(center)
			, m_Size(size)
		{}
		Vector2i m_Center;
		i32 m_Size;
	};

	std::queue<TileNode> queue;
	queue.emplace(Vector2i(shadowMapSize >> 1), shadowMapSize);

	for (u32 nodeIndex = 0; true; queue.pop())
	{
		const TileNode& parentNode = queue.front();

		const i32 parentHalfSize = parentNode.m_Size >> 1;
		const Vector2i parentTopLeft = parentNode.m_Center - parentHalfSize;

		m_pTiles[nodeIndex].m_TopLeftInPixels = Vector2u(parentTopLeft.m_X, parentTopLeft.m_Y);
		m_pTiles[nodeIndex].m_SizeInPixels = parentNode.m_Size;
		m_pTiles[nodeIndex].m_TexSpaceTopLeft = m_RcpMaxTileSize * Vector2f(parentTopLeft.m_X, parentTopLeft.m_Y);
		m_pTiles[nodeIndex].m_TexSpaceSize = m_RcpMaxTileSize * f32(parentNode.m_Size);

		++nodeIndex;

		if (nodeIndex == m_NumNodes)
			break;

		const i32 childHalfSize = parentNode.m_Size >> 2;
		queue.emplace(parentNode.m_Center + Vector2i(-childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2i( childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2i(-childHalfSize,  childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2i( childHalfSize,  childHalfSize), parentHalfSize);
	}
	Reset();
}

ShadowMapTileAllocator::~ShadowMapTileAllocator()
{
	SafeArrayDelete(m_pFreeNodes);
	SafeArrayDelete(m_pTiles);
}

bool ShadowMapTileAllocator::Allocate(u32 tileSize, ShadowMapTile* pNewTile)
{
	assert(pNewTile != nullptr);

	tileSize = Clamp(m_MinTileSize, m_MaxTileSize, tileSize);
	const u32 requiredLevel = CalcTreeLevel(tileSize);

	u32 rootLevel = 0;
	u32 rootIndex = 0;

	u32 foundIndex = FindFreeNode(rootLevel, rootIndex, requiredLevel);
	if (foundIndex != INVALID_INDEX)
	{
		m_pFreeNodes[foundIndex] = 0;
		*pNewTile = m_pTiles[foundIndex];

		return true;
	}
	return false;
}

void ShadowMapTileAllocator::Reset()
{
	std::memset(m_pFreeNodes, 1, m_NumNodes * sizeof(u8));
}

u32 ShadowMapTileAllocator::CalcTreeLevel(u32 tileSize) const
{
	return u32(m_Log2MaxTileSize - Ceil(Log2(f32(tileSize))));
}

u32 ShadowMapTileAllocator::FindFreeNode(u32 currentLevel, u32 currentIndex, u32 requiredLevel)
{
	if (m_pFreeNodes[currentIndex] == 1)
	{
		if (currentLevel == requiredLevel)
			return currentIndex;

		const u32 firstChildIndex = ((currentIndex << 2) + 1);
		const u32 lastChildIndex = firstChildIndex + 4;

		for (u32 childIndex = firstChildIndex; childIndex < lastChildIndex; ++childIndex)
		{
			u32 foundIndex = FindFreeNode(currentLevel + 1, childIndex, requiredLevel);
			if (foundIndex != INVALID_INDEX)
				return foundIndex;
		}
	}
	return INVALID_INDEX;
}