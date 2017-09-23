#include "Common/SceneLoader.h"
#include "Common/MeshUtilities.h"
#include "Common/OBJFileLoader.h"
#include "Common/Light.h"
#include "Common/Scene.h"

Scene* SceneLoader::LoadCube()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Cube\\cube.obj";

	OBJFileLoader fileLoader;
	Scene* pScene = fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);

	PointLight* pPointLight = new PointLight("Point light", 3.0f);
	pPointLight->SetColor(Vector3f(1.0f, 1.0f, 0.0f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(0.0f, 1.0f, 0.0f));

	pScene->AddPointLight(pPointLight);

	return pScene;
}

Scene* SceneLoader::LoadSponza()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Sponza\\sponza.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}
