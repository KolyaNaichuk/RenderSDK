#include "Common/SceneLoader.h"
#include "Common/Scene.h"
#include "Common/Mesh.h"
#include "Common/MeshBatch.h"
#include "Common/MeshUtilities.h"
#include "Common/Light.h"
#include "Common/Color.h"
#include "Common/Material.h"
#include "Common/OBJFileLoader.h"
#include "Math/BasisAxes.h"

Scene* SceneLoader::LoadCube()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Cube\\cube.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}

Scene* SceneLoader::LoadSponza()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Sponza\\sponza.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}
