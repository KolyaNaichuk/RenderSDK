#include "RenderPasses/SunShadowMapRenderer.h"

// 1.Construct light cascade frustum per each view frustum slice
// 2.Per each light cascade frustum detect visible meshes
// Since each light cascade frustum has ortho projection it could be represented as OOB
// Consequently, overlap test light frustum VS mesh could be performed as OOB VS OOB/AABB
// 3.Detect Min/Max Z for view frustum to calculate cascade splits
// 4.To speed up, average occluder distance I could use depth extent map (contains min/max depth value computed over region of shadow map)
// see Pasha's Fast Soft Shadows via Adaptive Shadow Maps 