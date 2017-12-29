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

	Camera* pCamera = new Camera(Camera::ProjType_Perspective, 0.1f/*nearClipPlane*/, 100.0f/*farClipPlane*/, 1.0f/*aspectRatio*/);
	pCamera->GetTransform().SetPosition(Vector3f(1.98481f, 2.67755f, -2.91555f));
	pCamera->GetTransform().SetRotation(Quaternion(0.312956f, -0.25733f, 0.0987797f, 0.908892f));
	
	pScene->SetCamera(pCamera);
	
	PointLight* pPointLight = new PointLight("Point light", 10.0f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(1.98481f, 2.67755f, -2.91555f));
	pPointLight->GetTransform().SetRotation(Quaternion(0.312956f, -0.25733f, 0.0987797f, 0.908892f));
		
	pScene->AddPointLight(pPointLight);

	SpotLight* pSpotLight = new SpotLight("Spot light", 5.0f, PI_DIV_4, PI_DIV_2);
	pSpotLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight->SetIntensity(1.0f);
	pSpotLight->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, -1.0f));

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
	Scene* pScene = fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
		
	Camera* pCamera = new Camera(Camera::ProjType_Perspective, 0.1f/*nearClipPlane*/, 2500.0f/*farClipPlane*/, 1.0f/*aspectRatio*/);
	pCamera->GetTransform().SetPosition(Vector3f(920.481f, 204.495f, 38.693f));
	pCamera->GetTransform().SetRotation(Quaternion(0.0f, 0.707107f, 0.0f, -0.707107f));
	pScene->SetCamera(pCamera);

	PointLight* pPointLight = new PointLight("Point light", 2000.0f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(-60.2261f, 1005.49f, 44.3329f));
	pScene->AddPointLight(pPointLight);
		
	return pScene;
}

Scene* SceneLoader::LoadSibenik()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\Sibenik\\sibenik.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}

Scene* SceneLoader::LoadCornellBox()
{
	const wchar_t* pathToOBJFile = L"..\\..\\Resources\\CornellBox\\CornellBox-Sphere.obj";

	OBJFileLoader fileLoader;
	return fileLoader.Load(pathToOBJFile, false/*use32BitIndices*/, ConvertionFlag_LeftHandedCoordSystem);
}
