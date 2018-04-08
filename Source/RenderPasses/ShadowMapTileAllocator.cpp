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
		TileNode(const Vector2f& center, f32 size)
			: m_Center(center)
			, m_Size(size)
		{}
		Vector2f m_Center;
		f32 m_Size;
	};

	std::queue<TileNode> queue;
	queue.emplace(Vector2f(0.5f * shadowMapSizeF), shadowMapSizeF);

	for (u32 nodeIndex = 0; true; queue.pop())
	{
		const TileNode& parentNode = queue.front();

		const f32 parentHalfSize = 0.5f * parentNode.m_Size;
		m_pTiles[nodeIndex].m_TexSpaceTopLeft = m_RcpMaxTileSize * (parentNode.m_Center - parentHalfSize);
		m_pTiles[nodeIndex].m_TexSpaceSize = m_RcpMaxTileSize * parentNode.m_Size;
		++nodeIndex;

		if (nodeIndex == m_NumNodes)
			break;

		const f32 childHalfSize = 0.25f * parentNode.m_Size;
		queue.emplace(parentNode.m_Center + Vector2f(-childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f(childHalfSize, -childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f(-childHalfSize, childHalfSize), parentHalfSize);
		queue.emplace(parentNode.m_Center + Vector2f(childHalfSize, childHalfSize), parentHalfSize);
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

		pNewTile->m_TexSpaceTopLeft = m_pTiles[foundIndex].m_TexSpaceTopLeft;
		pNewTile->m_TexSpaceSize = f32(tileSize) * m_RcpMaxTileSize;

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