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

	Camera* pCamera = new Camera(
		Camera::ProjType_Perspective,
		0.1f/*nearClipPlane*/,
		100.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		0.3f/*maxMoveSpeed*/,
		0.1f/*maxRotationSpeed*/);
	
	pCamera->GetTransform().SetPosition(Vector3f(1.98481f, 2.67755f, -2.91555f));
	pCamera->GetTransform().SetRotation(Quaternion(0.312956f, -0.25733f, 0.0987797f, 0.908892f));
	
	pScene->SetCamera(pCamera);
	
	PointLight* pPointLight = new PointLight("Point light", 10.0f, 0.1f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(1.98481f, 2.67755f, -2.91555f));
	pPointLight->GetTransform().SetRotation(Quaternion(0.312956f, -0.25733f, 0.0987797f, 0.908892f));
		
	pScene->AddPointLight(pPointLight);

	SpotLight* pSpotLight = new SpotLight("Spot light", 5.0f, PI_DIV_4, PI_DIV_2, 0.1f);
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
		
	Camera* pCamera = new Camera(Camera::ProjType_Perspective,
		1.5f/*nearClipPlane*/,
		3800.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		3.0f/*maxMoveSpeed*/,
		2.0f/*maxRotationSpeed*/);

	pCamera->GetTransform().SetPosition(Vector3f(1190.48f, 204.495f, 38.693f));
	pCamera->GetTransform().SetRotation(Quaternion(0.0f, 0.707107f, 0.0f, -0.707107f));
	pScene->SetCamera(pCamera);
	
	//Vector3f minPoint(-1920.94592f, -126.442497f, -1105.42603f);
	//Vector3f maxPoint(1799.90808f, 1429.43323f, 1182.80713f);
	//Vector3f size = maxPoint - minPoint;
	//Vector3f halfSize = 0.5f * size;
	
#if 1
	PointLight* pPointLight = new PointLight("Point light", 1900.0f, 0.1f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pScene->AddPointLight(pPointLight);
#endif

#if 1
	SpotLight* pSpotLight = new SpotLight("Spot Light", 780.0f, ToRadians(45.0f), ToRadians(60.0f), 0.1f);
	pSpotLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight->SetIntensity(1.0f);
	pSpotLight->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight->GetTransform().SetRotation(CreateRotationXQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight);
#endif

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
