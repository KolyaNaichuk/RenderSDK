#include "Scene/SceneLoader.h"
#include "Scene/OBJFileLoader.h"
#include "Scene/Light.h"
#include "Scene/Scene.h"
#include "D3DWrapper/Common.h"

Scene* SceneLoader::LoadCube()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\Cube\\cube.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\Cube\\cube.obj";
#endif
	Scene* pScene = LoadSceneFromOBJFile(pFilePath);
	
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

	SpotLight* pSpotLight = new SpotLight("Spot light", 5.0f, PI_DIV_4, PI_DIV_2, 0.1f, 80.0f);
	pSpotLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight->SetIntensity(1.0f);
	pSpotLight->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, -1.0f));

	pScene->AddSpotLight(pSpotLight);

	return pScene;
}

Scene* SceneLoader::LoadErato()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\Erato\\erato-1.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\Erato\\erato-1.obj";
#endif
	Scene* pScene = LoadSceneFromOBJFile(pFilePath);
	return pScene;
}

Scene* SceneLoader::LoadCrytekSponza()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\CrytekSponza\\sponza.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\CrytekSponza\\sponza.obj";
#endif
	Scene* pScene = LoadSceneFromOBJFile(pFilePath);
	
	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius; // {-1920.94592f, -126.442497f, -1105.42603f}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius; // {1799.90808f, 1429.43323f, 1182.80713f}
		
	Camera* pCamera = new Camera(Camera::ProjType_Perspective,
		1.5f/*nearClipPlane*/,
		3800.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		3.0f/*maxMoveSpeed*/,
		2.0f/*maxRotationSpeed*/);

	pCamera->GetTransform().SetPosition(Vector3f(1190.48f, 204.495f, 38.693f));
	pCamera->GetTransform().SetRotation(Quaternion(0.0f, 0.707107f, 0.0f, -0.707107f));
	pScene->SetCamera(pCamera);

#if 1
	SpotLight* pSpotLight1 = new SpotLight("Spot Light 1", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight1->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight1->SetIntensity(1.0f);
	pSpotLight1->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pScene->AddSpotLight(pSpotLight1);
#endif

#if 1
	SpotLight* pSpotLight2 = new SpotLight("Spot Light 2", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight2->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight2->SetIntensity(1.0f);
	pSpotLight2->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight2->GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight2);
#endif

#if 1
	SpotLight* pSpotLight3 = new SpotLight("Spot Light 3", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight3->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight3->SetIntensity(1.0f);
	pSpotLight3->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight3->GetTransform().SetRotation(CreateRotationYQuaternion(-PI_DIV_2));
	pScene->AddSpotLight(pSpotLight3);
#endif

#if 1
	SpotLight* pSpotLight4 = new SpotLight("Spot Light 4", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight4->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight4->SetIntensity(1.0f);
	pSpotLight4->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight4->GetTransform().SetRotation(CreateRotationYQuaternion(PI));
	pScene->AddSpotLight(pSpotLight4);
#endif

#if 1
	SpotLight* pSpotLight5 = new SpotLight("Spot Light 5", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight5->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight5->SetIntensity(1.0f);
	pSpotLight5->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight5->GetTransform().SetRotation(CreateRotationXQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight5);
#endif

#if 1
	SpotLight* pSpotLight6 = new SpotLight("Spot Light 6", 1900.0f, ToRadians(60.0f), ToRadians(70.0f), 0.1f, 80.0f);
	pSpotLight6->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight6->SetIntensity(1.0f);
	pSpotLight6->GetTransform().SetPosition(Vector3f(-61.0f, 652.0f, 39.0f));
	pSpotLight6->GetTransform().SetRotation(CreateRotationXQuaternion(-PI_DIV_2));
	pScene->AddSpotLight(pSpotLight6);
#endif

	return pScene;
}

Scene* SceneLoader::LoadDabrovicSponza()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\\..\\..\\Resources\\DabrovicSponza\\sponza.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\DabrovicSponza\\sponza.obj";
#endif
	Scene* pScene = LoadSceneFromOBJFile(pFilePath);

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius; // {-17.4027596, -0.906688690, -7.80148792}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius; // {17.4172401, 15.6533108, 7.79851246}

	Camera* pCamera = new Camera(Camera::ProjType_Perspective,
		0.1f/*nearClipPlane*/,
		50.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		0.1f/*maxMoveSpeed*/,
		0.4f/*maxRotationSpeed*/);

	pCamera->GetTransform().SetPosition(Vector3f(-10.0f, 7.0f, 0.0f));
	pCamera->GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));
	pScene->SetCamera(pCamera);

#if 1
	PointLight* pPointLight = new PointLight("Point light", 35.0f, 0.1f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(0.0f, 7.5f, 0.0f));
	pScene->AddPointLight(pPointLight);
#endif
	
	return pScene;
}

Scene* SceneLoader::LoadSibenik()
{
#ifdef ENABLE_EXTERNAL_TOOL_DEBUGGING
	const wchar_t* pFilePath = L"..\..\\..\\Resources\\Sibenik\\sibenik.obj";
#else
	const wchar_t* pFilePath = L"..\\..\\Resources\\Sibenik\\sibenik.obj";
#endif
	Scene* pScene = LoadSceneFromOBJFile(pFilePath, true/*use32BitIndices*/);

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius; // {-20.1410999, -15.3123074, -8.49680042}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius; // {20.1410999, 15.3000011, 8.49680042}

	Camera* pCamera = new Camera(Camera::ProjType_Perspective,
		1.0f/*nearClipPlane*/,
		45.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		0.4f/*maxMoveSpeed*/,
		0.8f/*maxRotationSpeed*/);

	pCamera->GetTransform().SetPosition(Vector3f(-19.7f, -11.0f, 0.0f));
	pCamera->GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));
	pScene->SetCamera(pCamera);

#if 1
	SpotLight* pSpotLight1 = new SpotLight("Spot Light 1", 50.0f/*range*/, ToRadians(45.0f)/*innerConeAngleInRadians*/,
		ToRadians(70.0f)/*outerConeAngleInRadians*/, 0.1f/*shadowNearPlane*/, 80.0f/*expShadowMapConstant*/);
	pSpotLight1->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight1->SetIntensity(1.0f);
	pSpotLight1->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pSpotLight1->GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));

	pScene->AddSpotLight(pSpotLight1);
#endif

#if 0
	PointLight* pPointLight1 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight1->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight1->SetIntensity(1.0f);
	pPointLight1->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight1);
#endif

#if 0
	PointLight* pPointLight2 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight2->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight2->SetIntensity(1.0f);
	pPointLight2->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight2);
#endif

#if 0
	PointLight* pPointLight3 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight3->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight3->SetIntensity(1.0f);
	pPointLight3->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight3);
#endif

#if 0
	PointLight* pPointLight4 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight4->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight4->SetIntensity(1.0f);
	pPointLight4->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight4);
#endif

#if 0
	PointLight* pPointLight5 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight5->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight5->SetIntensity(1.0f);
	pPointLight5->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight5);
#endif

#if 0
	PointLight* pPointLight6 = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight6->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight6->SetIntensity(1.0f);
	pPointLight6->GetTransform().SetPosition(Vector3f(0.0f, 0.0f, 0.0f));
	pScene->AddPointLight(pPointLight6);
#endif

	return pScene;
}

Scene* SceneLoader::LoadCornellBox()
{
	Scene* pScene = LoadSceneFromOBJFile(L"..\\..\\Resources\\CornellBox\\CornellBox-Sphere.obj");

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius;
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius;

	return pScene;
}

Scene* SceneLoader::LoadSanMiguel()
{
#if 0
	Scene* pScene = LoadSceneFromOBJFile(L"..\\..\\Resources\\SanMiguel\\san-miguel-low-poly.obj", true/*use32BitIndices*/);
#else
	Scene* pScene = LoadSceneFromOBJFile(L"..\\..\\Resources\\SanMiguel\\san-miguel.obj", true/*use32BitIndices*/);
#endif

	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius; // {-22.2742996, -0.269336700, -14.9373989}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius; // {46.7743988, 14.6000004, 12.0422001}

	Camera* pCamera = new Camera(Camera::ProjType_Perspective,
		0.2f/*nearClipPlane*/,
		80.0f/*farClipPlane*/,
		1.0f/*aspectRatio*/,
		0.2f/*maxMoveSpeed*/,
		0.4f/*maxRotationSpeed*/);

	pCamera->GetTransform().SetPosition(Vector3f(12.0f, 7.0f, 0.0f));
	pCamera->GetTransform().SetRotation(CreateRotationYQuaternion(PI_DIV_2));
	pScene->SetCamera(pCamera);

#if 1
	PointLight* pPointLight = new PointLight("Point light", 50.0f, 0.1f);
	pPointLight->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pPointLight->SetIntensity(1.0f);
	pPointLight->GetTransform().SetPosition(Vector3f(12.0f, 7.0f, 0.0f));
	pScene->AddPointLight(pPointLight);
#endif

	return pScene;
}

