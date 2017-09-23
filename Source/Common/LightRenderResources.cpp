#include "Common/LightRenderResources.h"
#include "Common/Light.h"
#include "D3DWrapper/GraphicsResource.h"
#include "D3DWrapper/RenderEnv.h"
#include "D3DWrapper/CommandList.h"
#include "Math/Cone.h"
#include "Math/Transform.h"
#include "Math/Quaternion.h"
#include "Math/Frustum.h"

struct PointLightProps
{
	PointLightProps(const Vector3f& color, f32 attenStartRange)
		: m_Color(color)
		, m_AttenStartRange(attenStartRange)
	{}
	Vector3f m_Color;
	f32 m_AttenStartRange;
};

struct SpotLightProps
{
	SpotLightProps(const Vector3f& color, const Vector3f& worldSpaceDir, f32 attenStartRange, f32 attenEndRange, f32 cosHalfInnerConeAngle, f32 cosHalfOuterConeAngle)
		: m_Color(color)
		, m_WorldSpaceDir(worldSpaceDir)
		, m_AttenStartRange(attenStartRange)
		, m_AttenEndRange(attenEndRange)
		, m_CosHalfInnerConeAngle(cosHalfInnerConeAngle)
		, m_CosHalfOuterConeAngle(cosHalfOuterConeAngle)
	{}
	Vector3f m_Color;
	Vector3f m_WorldSpaceDir;
	f32 m_AttenStartRange;
	f32 m_AttenEndRange;
	f32 m_CosHalfInnerConeAngle;
	f32 m_CosHalfOuterConeAngle;
};

struct LightFrustum
{
	LightFrustum(const Plane& leftPlane, const Plane& rightPlane, const Plane& topPlane, const Plane& bottomPlane)
		: m_LeftPlane(leftPlane)
		, m_RightPlane(rightPlane)
		, m_TopPlane(topPlane)
		, m_BottomPlane(bottomPlane)
	{}
	Plane m_LeftPlane;
	Plane m_RightPlane;
	Plane m_TopPlane;
	Plane m_BottomPlane;
};

LightRenderResources::LightRenderResources(RenderEnv* pRenderEnv, u32 numPointLights, PointLight** ppPointLights)
	: m_NumLights(numPointLights)
	, m_pLightWorldBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
	, m_pLightViewProjMatrixBuffer(nullptr)
{
	std::vector<Sphere> lightWorldBounds;
	lightWorldBounds.reserve(m_NumLights);

	std::vector<PointLightProps> lightProps;
	lightProps.reserve(m_NumLights);

	std::vector<LightFrustum> lightFrustums;
	lightFrustums.reserve(kNumCubeMapFaces * m_NumLights);

	std::vector<Matrix4f> lightViewProjMatrices;	
	lightViewProjMatrices.reserve(kNumCubeMapFaces * m_NumLights);

	Quaternion cubeMapWorldSpaceRotations[kNumCubeMapFaces];
	cubeMapWorldSpaceRotations[kCubeMapFacePosX] = CreateRotationYQuaternion(PI_DIV_2);
	cubeMapWorldSpaceRotations[kCubeMapFaceNegX] = CreateRotationYQuaternion(-PI_DIV_2);
	cubeMapWorldSpaceRotations[kCubeMapFacePosY] = CreateRotationXQuaternion(-PI_DIV_2);
	cubeMapWorldSpaceRotations[kCubeMapFaceNegY] = CreateRotationXQuaternion(PI_DIV_2);
	cubeMapWorldSpaceRotations[kCubeMapFacePosZ] = Quaternion();
	cubeMapWorldSpaceRotations[kCubeMapFaceNegZ] = CreateRotationYQuaternion(PI);
		
	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();
		const Quaternion& lightWorldSpaceRotation = lightWorldSpaceTransform.GetRotation();
		
		lightWorldBounds.emplace_back(lightWorldSpacePos, pLight->GetAttenEndRange());
		lightProps.emplace_back(pLight->GetColor(), pLight->GetAttenStartRange());

		Matrix4f lightProjMatrix = CreatePerspectiveFovProjMatrix(PI_DIV_2, 1.0f, 0.001f, pLight->GetAttenEndRange());
		Frustum lightSpaceFrustum(lightProjMatrix);
		
		for (u8 faceIndex = 0; faceIndex < kNumCubeMapFaces; ++faceIndex)
		{
			Transform cubeMapLightWorldSpaceTransform(cubeMapWorldSpaceRotations[faceIndex] * lightWorldSpaceRotation, lightWorldSpacePos);
			Frustum lightWorldSpaceFrustum = TransformFrustum(lightSpaceFrustum, cubeMapLightWorldSpaceTransform);

			const Plane& leftWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::LeftPlane];
			const Plane& rightWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::RightPlane];
			const Plane& topWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::TopPlane];
			const Plane& bottomWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::BottomPlane];

			lightFrustums.emplace_back(leftWorldSpacePlane, rightWorldSpacePlane, topWorldSpacePlane, bottomWorldSpacePlane);

			Matrix4f lightViewProjMatrix = cubeMapLightWorldSpaceTransform.GetWorldToLocalMatrix() * lightProjMatrix;
			lightViewProjMatrices.emplace_back(lightViewProjMatrix);
		}
	}
		
	StructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	m_pLightWorldBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightWorldBoundsBuffer");
	UploadData(pRenderEnv, m_pLightWorldBoundsBuffer, lightBoundsBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightWorldBounds.data(), m_NumLights * sizeof(Sphere));
	
	StructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(PointLightProps), true, false);
	m_pLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightPropsBuffer");
	UploadData(pRenderEnv, m_pLightPropsBuffer, lightPropsBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightProps.data(), m_NumLights * sizeof(PointLightProps));

	StructuredBufferDesc lightFrustumBufferDesc(kNumCubeMapFaces * m_NumLights, sizeof(LightFrustum), true, false);
	m_pLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightFrustumBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightFrustumBuffer");
	UploadData(pRenderEnv, m_pLightFrustumBuffer, lightFrustumBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightFrustums.data(), kNumCubeMapFaces * m_NumLights * sizeof(LightFrustum));

	StructuredBufferDesc lightViewProjMatrixBufferDesc(kNumCubeMapFaces * m_NumLights, sizeof(Matrix4f), true, false);
	m_pLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightViewProjMatrixBuffer");
	UploadData(pRenderEnv, m_pLightViewProjMatrixBuffer, lightViewProjMatrixBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightViewProjMatrices.data(), kNumCubeMapFaces * m_NumLights * sizeof(Matrix4f));
}

LightRenderResources::LightRenderResources(RenderEnv* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights)
	: m_NumLights(numSpotLights)
	, m_pLightWorldBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
	, m_pLightViewProjMatrixBuffer(nullptr)
{
	std::vector<Sphere> lightWorldBounds;
	lightWorldBounds.reserve(m_NumLights);

	std::vector<SpotLightProps> lightProps;
	lightProps.reserve(m_NumLights);

	std::vector<LightFrustum> lightFrustums;
	lightFrustums.reserve(m_NumLights);

	std::vector<Matrix4f> lightViewProjMatrices;
	lightViewProjMatrices.reserve(m_NumLights);
		
	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const SpotLight* pLight = ppSpotLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();

		const BasisAxes lightWorldSpaceBasis = ExtractBasisAxes(lightWorldSpaceTransform.GetRotation());
		
		assert(IsNormalized(lightWorldSpaceBasis.m_ZAxis));
		const Vector3f& lightWorldSpaceDir = lightWorldSpaceBasis.m_ZAxis;

		Cone cone(lightWorldSpacePos, pLight->GetOuterConeAngle(), lightWorldSpaceDir, pLight->GetAttenEndRange());
		lightWorldBounds.emplace_back(ExtractBoundingSphere(cone));
				
		f32 cosHalfInnerConeAngle = Cos(0.5f * pLight->GetInnerConeAngle());
		f32 cosHalfOuterConeAngle = Cos(0.5f * pLight->GetOuterConeAngle());
		
		lightProps.emplace_back(pLight->GetColor(), lightWorldSpaceDir, pLight->GetAttenStartRange(), pLight->GetAttenEndRange(), cosHalfInnerConeAngle, cosHalfOuterConeAngle);

		Matrix4f lightProjMatrix = CreatePerspectiveFovProjMatrix(pLight->GetOuterConeAngle(), 1.0f, 0.001f, pLight->GetAttenEndRange());
		
		Frustum lightSpaceFrustum(lightProjMatrix);
		Frustum lightWorldSpaceFrustum = TransformFrustum(lightSpaceFrustum, lightWorldSpaceTransform);
		
		const Plane& leftWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::LeftPlane];
		const Plane& rightWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::RightPlane];
		const Plane& topWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::TopPlane];
		const Plane& bottomWorldSpacePlane = lightWorldSpaceFrustum.m_Planes[Frustum::BottomPlane];
		
		lightFrustums.emplace_back(leftWorldSpacePlane, rightWorldSpacePlane, topWorldSpacePlane, bottomWorldSpacePlane);
			
		Matrix4f lightViewProjMatrix = lightWorldSpaceTransform.GetWorldToLocalMatrix() * lightProjMatrix;
		lightViewProjMatrices.emplace_back(lightViewProjMatrix);
	}

	StructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	m_pLightWorldBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightWorldBoundsBuffer");
	UploadData(pRenderEnv, m_pLightWorldBoundsBuffer, lightBoundsBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightWorldBounds.data(), m_NumLights * sizeof(Sphere));

	StructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(SpotLightProps), true, false);
	m_pLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightPropsBuffer");
	UploadData(pRenderEnv, m_pLightPropsBuffer, lightPropsBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightProps.data(), m_NumLights * sizeof(SpotLightProps));
	
	StructuredBufferDesc lightFrustumBufferDesc(m_NumLights, sizeof(LightFrustum), true, false);
	m_pLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightFrustumBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightFrustumBuffer");
	UploadData(pRenderEnv, m_pLightFrustumBuffer, lightFrustumBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightFrustums.data(), m_NumLights * sizeof(LightFrustum));

	StructuredBufferDesc lightViewProjMatrixBufferDesc(m_NumLights, sizeof(Matrix4f), true, false);
	m_pLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps,
		&lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightRenderResources::m_pLightViewProjMatrixBuffer");
	UploadData(pRenderEnv, m_pLightViewProjMatrixBuffer, lightViewProjMatrixBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, lightViewProjMatrices.data(), m_NumLights * sizeof(Matrix4f));
}

LightRenderResources::~LightRenderResources()
{
	SafeDelete(m_pLightWorldBoundsBuffer);
	SafeDelete(m_pLightPropsBuffer);
	SafeDelete(m_pLightFrustumBuffer);
	SafeDelete(m_pLightViewProjMatrixBuffer);
}