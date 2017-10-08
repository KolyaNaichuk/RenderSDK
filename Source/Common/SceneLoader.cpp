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

	PointLight* pPointLight = new PointLight("Point light", 5.0f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(0.0f, 2.0f, -2.5f));

	pScene->AddPointLight(pPointLight);

	SpotLight* pSpotLight = new SpotLight("Spot light", 5.0f, PI_DIV_4, PI_DIV_2);
	pSpotLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight->SetIntensity(1.0f);
	pSpotLight->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, -1.05f));
	
	pScene->AddSpotLight(pSpotLight);

	return pScene;
}

Scene* SceneLoader::LoadErato()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Erato\\erato-1.obj";

	OBJFileLoader fileLoader;
	Scene* pScene = fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);

	return pScene;
}

Scene* SceneLoader::LoadSponza()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Sponza\\sponza.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}
