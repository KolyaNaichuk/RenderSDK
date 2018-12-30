#include "Scene/SceneLoader.h"
#include "D3DWrapper/Common.h"
#include "Math/Transform.h"
#include "Scene/OBJFileLoader.h"
#include "Scene/Light.h"
#include "Scene/Scene.h"

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
		Vector3f(0.2f)/*rotationSpeed*/
	);
	pScene->SetCamera(pCamera);

	SpotLight* pSpotLight1 = new SpotLight(
		Vector3f(-894, 400.0f, 39.0f)/*worldPosition*/,
		BasisAxes(CreateRotationMatrix(CreateRotationXQuaternion(PI_DIV_2)))/*worldOrientation*/,
		Vector3f(1.43f, 1.43f, 1.43f)/*radiantPower*/,
		600.0f/*range*/,
		ToRadians(45.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight1);

	SpotLight* pSpotLight2 = new SpotLight(
		Vector3f(-93.0f, 400.0f, 39.0f)/*worldPosition*/,
		BasisAxes(CreateRotationMatrix(CreateRotationXQuaternion(PI_DIV_2)))/*worldOrientation*/,
		Vector3f(1.43f, 1.43f, 1.43f)/*radiantPower*/,
		600.0f/*range*/,
		ToRadians(45.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight2);

	SpotLight* pSpotLight3 = new SpotLight(
		Vector3f(708.0f, 400.0f, 39.0f)/*worldPosition*/,
		BasisAxes(CreateRotationMatrix(CreateRotationXQuaternion(PI_DIV_2)))/*worldOrientation*/,
		Vector3f(1.43f, 1.43f, 1.43f)/*radiantPower*/,
		600.0f/*range*/,
		ToRadians(45.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight3);
	
	SpotLight* pSpotLight4 = new SpotLight(
		Vector3f(-800.0f, 600.0f, 39.0f)/*worldPosition*/,
		BasisAxes(CreateRotationMatrix(CreateRotationYQuaternion(-PI_DIV_2)))/*worldOrientation*/,
		Vector3f(2.45f, 2.45f, 2.45f)/*radiantPower*/,
		700.0f/*range*/,
		ToRadians(90.0f)/*innerConeAngleInRadians*/,
		ToRadians(120.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight4);
 
	SpotLight* pSpotLight5 = new SpotLight(
		Vector3f(-800.0f, 600.0f, 39.0f)/*worldPosition*/,
		BasisAxes()/*worldOrientation*/,
		Vector3f(2.45f, 2.45f, 2.45f)/*radiantPower*/,
		700.0f/*range*/,
		ToRadians(90.0f)/*innerConeAngleInRadians*/,
		ToRadians(120.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight5);

	SpotLight* pSpotLight6 = new SpotLight(
		Vector3f(-800.0f, 600.0f, 39.0f)/*worldPosition*/,
		BasisAxes(CreateRotationMatrix(CreateRotationYQuaternion(PI)))/*worldOrientation*/,
		Vector3f(2.45f, 2.45f, 2.45f)/*radiantPower*/,
		700.0f/*range*/,
		ToRadians(90.0f)/*innerConeAngleInRadians*/,
		ToRadians(120.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight6);
		
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
	
	return pScene;
}
