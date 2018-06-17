#include "Scene/SceneLoader.h"
#include "Scene/OBJFileLoader.h"
#include "Scene/Light.h"
#include "Scene/Scene.h"
#include "D3DWrapper/Common.h"

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

	Camera* pCamera = new Camera(
		Vector3f(1190.48f, 204.495f, 38.693f)/*position*/,
		BasisAxes(Vector3f::FORWARD, Vector3f::UP, Vector3f::LEFT),
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		1.5f/*nearClipDist*/,
		3800.0f/*farClipDist*/,
		Vector3f(1.0f)/*moveSpeed*/,
		Vector3f(0.2f)/*rotationSpeed*/);

	pScene->SetCamera(pCamera);

	SpotLight* pSpotLight1 = new SpotLight("Spot Light 1", 600.0f, ToRadians(45.0f), ToRadians(90.0f), 0.1f, 80.0f);
	pSpotLight1->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight1->SetIntensity(1.0f);
	pSpotLight1->GetTransform().SetPosition(Vector3f(-894, 400.0f, 39.0f));
	pSpotLight1->GetTransform().SetRotation(CreateRotationXQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight1);

	SpotLight* pSpotLight2 = new SpotLight("Spot Light 2", 600.0f, ToRadians(45.0f), ToRadians(90.0f), 0.1f, 80.0f);
	pSpotLight2->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight2->SetIntensity(1.0f);
	pSpotLight2->GetTransform().SetPosition(Vector3f(-93.0f, 400.0f, 39.0f));
	pSpotLight2->GetTransform().SetRotation(CreateRotationXQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight2);

	SpotLight* pSpotLight3 = new SpotLight("Spot Light 3", 600.0f, ToRadians(45.0f), ToRadians(90.0f), 0.1f, 80.0f);
	pSpotLight3->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight3->SetIntensity(1.0f);
	pSpotLight3->GetTransform().SetPosition(Vector3f(708.0f, 400.0f, 39.0f));
	pSpotLight3->GetTransform().SetRotation(CreateRotationXQuaternion(PI_DIV_2));
	pScene->AddSpotLight(pSpotLight3);
	
	SpotLight* pSpotLight4 = new SpotLight("Spot Light 4", 700.0f, ToRadians(90.0f), ToRadians(120.0f), 0.1f, 80.0f);
	pSpotLight4->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight4->SetIntensity(1.0f);
	pSpotLight4->GetTransform().SetPosition(Vector3f(-800.0f, 600.0f, 39.0f));
	pSpotLight4->GetTransform().SetRotation(CreateRotationYQuaternion(-PI_DIV_2));
	pScene->AddSpotLight(pSpotLight4);

	SpotLight* pSpotLight5 = new SpotLight("Spot Light 5", 700.0f, ToRadians(90.0f), ToRadians(120.0f), 0.1f, 80.0f);
	pSpotLight5->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight5->SetIntensity(1.0f);
	pSpotLight5->GetTransform().SetPosition(Vector3f(-800.0f, 600.0f, 39.0f));
	pScene->AddSpotLight(pSpotLight5);

	SpotLight* pSpotLight6 = new SpotLight("Spot Light 6", 700.0f, ToRadians(90.0f), ToRadians(120.0f), 0.1f, 80.0f);
	pSpotLight6->SetColor(Vector3f(0.78f, 0.78f, 0.78f));
	pSpotLight6->SetIntensity(1.0f);
	pSpotLight6->GetTransform().SetPosition(Vector3f(-800.0f, 600.0f, 39.0f));
	pSpotLight6->GetTransform().SetRotation(CreateRotationYQuaternion(PI));
	pScene->AddSpotLight(pSpotLight6);
		
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
	
	Camera* pCamera = new Camera(
		Vector3f(-10.0f, 7.0f, 0.0f)/*position*/,
		BasisAxes(Vector3f::BACK, Vector3f::UP, Vector3f::RIGHT),
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		0.1f/*nearClipDist*/,
		50.0f/*farClipDist*/,
		Vector3f(0.1f)/*moveSpeed*/,
		Vector3f(0.4f)/*rotationSpeed*/);

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

	Camera* pCamera = new Camera(
		Vector3f(-19.7f, -11.0f, 0.0f)/*position*/,
		BasisAxes(Vector3f::BACK, Vector3f::UP, Vector3f::RIGHT),
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		1.0f/*nearClipDist*/,
		45.0f/*farClipDist*/,
		Vector3f(0.4f)/*moveSpeed*/,
		Vector3f(0.8f)/*rotationSpeed*/);

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
	
	Camera* pCamera = new Camera(
		Vector3f(12.0f, 7.0f, 0.0f)/*position*/,
		BasisAxes(Vector3f::BACK, Vector3f::UP, Vector3f::RIGHT),
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		0.2f/*nearClipDist*/,
		80.0f/*farClipDist*/,
		Vector3f(0.2f)/*moveSpeed*/,
		Vector3f(0.4f)/*rotationSpeed*/);

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

