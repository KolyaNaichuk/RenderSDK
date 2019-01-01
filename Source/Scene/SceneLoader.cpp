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
	Matrix4f matrix1 = CreateTranslationMatrix(60.5189209f, -651.495361f, -38.6905518f);
	Matrix4f matrix2 = CreateScalingMatrix(0.01f);
	Matrix4f matrix3 = CreateRotationYMatrix(PI_DIV_2);
	Matrix4f matrix4 = CreateTranslationMatrix(0.0f, 7.8f, 18.7f);
	Matrix4f worldMatrix = matrix1 * matrix2 * matrix3 * matrix4;

	Scene* pScene = LoadSceneFromOBJFile(pFilePath, worldMatrix);
	
	const AxisAlignedBox& worldBounds = pScene->GetWorldBounds();
	// {-11.4411659, 0.020621, 0.095729}
	const Vector3f minPoint = worldBounds.m_Center - worldBounds.m_Radius;
	// {11.4411659, 15.5793781, 37.304271}
	const Vector3f maxPoint = worldBounds.m_Center + worldBounds.m_Radius;

	Camera* pCamera = new Camera(
		Vector3f(0.0f, 2.8f, 5.5f)/*worldPosition*/,
		BasisAxes()/*worldOrientation*/,
		PI_DIV_4/*fovYInRadians*/,
		1.0f/*aspectRatio*/,
		0.1f/*nearClipDist*/,
		60.0f/*farClipDist*/,
		Vector3f(0.04f)/*moveSpeed*/,
		Vector3f(1.2f)/*rotationSpeed*/
	);
	pScene->SetCamera(pCamera);

#if 1
	SpotLight* pSpotLight1 = new SpotLight(
		Vector3f(0.0f, 8.0f, 17.0f)/*worldPosition*/,
		BasisAxes(Vector3f::RIGHT, Vector3f::FORWARD, Vector3f::DOWN)/*worldOrientation*/,
		Vector3f(1.43f, 1.43f, 1.43f)/*radiantPower*/,
		12.0f/*range*/,
		ToRadians(45.0f)/*innerConeAngleInRadians*/,
		ToRadians(90.0f)/*outerConeAngleInRadians*/,
		0.1f/*shadowNearPlane*/,
		80.0f/*expShadowMapConstant*/
	);
	pScene->AddSpotLight(pSpotLight1);
#endif

#if 0
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
#endif

#if 0
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
#endif

// Front
#if 0
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
#endif

// Right half part
#if 0
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
#endif

// Left half space
#if 0
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
#endif
		
	return pScene;
}
