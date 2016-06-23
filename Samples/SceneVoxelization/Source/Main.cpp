/*
OpenGL Insights
Cyril Crassin and Simon Green, Octree-Based Sparse Voxelization Using the GPU Hardware Rasterization

https://en.wikipedia.org/wiki/Moving_average
Section on comulative moving average

https://developer.nvidia.com/content/basics-gpu-voxelization
The Basics of GPU Voxelization

GPU Pro 4:
Hawar Doghramachi, Rasterized Voxel-Based Dynamic Global Illumination

GPU Pro 6:
Hawar Doghramachi, Tile-Based Omnidirectional Shadows
*/

#include "DXApplication.h"

int APIENTRY wWinMain(HINSTANCE hApp, HINSTANCE hPrevApp, LPWSTR pCmdLine, int showCmd)
{
	UNREFERENCED_PARAMETER(hPrevApp);
	UNREFERENCED_PARAMETER(pCmdLine);

	DXApplication app(hApp);
	return app.Run(showCmd);
}