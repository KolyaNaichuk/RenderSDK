#include "Common/SceneLoader.h"
#include "Common/MeshUtilities.h"
#include "Common/OBJFileLoader.h"

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
