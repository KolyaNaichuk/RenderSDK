#include "Common/LightBuffer.h"
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

LightBuffer::LightBuffer(RenderEnv* pRenderEnv, u32 numPointLights, PointLight** ppPointLights)
	: m_NumLights(numPointLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pUploadLightFrustumBuffer(nullptr)
	, m_pUploadLightViewProjMatrixBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
	, m_pLightViewProjMatrixBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<PointLightProps> lightProps;
	std::vector<LightFrustum> lightFrustums;
	std::vector<Matrix4f> lightViewProjMatrices;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);
	lightFrustums.reserve(kNumCubeMapFaces * m_NumLights);
	lightViewProjMatrices.reserve(kNumCubeMapFaces * m_NumLights);

	Quaternion cubeMapWorldSpaceRotations[kNumCubeMapFaces];
	cubeMapWorldSpaceRotations[kCubeMapFacePosX] = CreateRotationYQuaternion(Radian(PI_DIV_TWO));
	cubeMapWorldSpaceRotations[kCubeMapFaceNegX] = CreateRotationYQuaternion(Radian(-PI_DIV_TWO));
	cubeMapWorldSpaceRotations[kCubeMapFacePosY] = CreateRotationXQuaternion(Radian(-PI_DIV_TWO));
	cubeMapWorldSpaceRotations[kCubeMapFaceNegY] = CreateRotationXQuaternion(Radian(PI_DIV_TWO));
	cubeMapWorldSpaceRotations[kCubeMapFacePosZ] = Quaternion();
	cubeMapWorldSpaceRotations[kCubeMapFaceNegZ] = CreateRotationYQuaternion(Radian(PI));
		
	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();
		const Quaternion& lightWorldSpaceRotation = lightWorldSpaceTransform.GetRotation();
		
		lightBounds.emplace_back(lightWorldSpacePos, pLight->GetAttenEndRange());
		lightProps.emplace_back(pLight->GetColor(), pLight->GetAttenStartRange());

		Matrix4f lightProjMatrix = CreatePerspectiveFovProjMatrix(Radian(PI_DIV_TWO), 1.0f, 0.001f, pLight->GetAttenEndRange());
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
	StructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(PointLightProps), true, false);
	StructuredBufferDesc lightFrustumBufferDesc(kNumCubeMapFaces * m_NumLights, sizeof(LightFrustum), true, false);
	StructuredBufferDesc lightViewProjMatrixBufferDesc(kNumCubeMapFaces * m_NumLights, sizeof(Matrix4f), true, false);

	m_pLightBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");
	m_pLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightFrustumBuffer");
	m_pLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightViewProjMatrixBuffer");

	m_pUploadLightBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(PointLightProps));

	m_pUploadLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightFrustumBuffer");
	m_pUploadLightFrustumBuffer->Write(lightFrustums.data(), kNumCubeMapFaces * m_NumLights * sizeof(LightFrustum));

	m_pUploadLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightViewProjMatrixBuffer");
	m_pUploadLightViewProjMatrixBuffer->Write(lightViewProjMatrices.data(), kNumCubeMapFaces * m_NumLights * sizeof(Matrix4f));
}

LightBuffer::LightBuffer(RenderEnv* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights)
	: m_NumLights(numSpotLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pUploadLightFrustumBuffer(nullptr)
	, m_pUploadLightViewProjMatrixBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
	, m_pLightViewProjMatrixBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<SpotLightProps> lightProps;
	std::vector<LightFrustum> lightFrustums;
	std::vector<Matrix4f> lightViewProjMatrices;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);
	lightFrustums.reserve(m_NumLights);
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
		lightBounds.emplace_back(ExtractBoundingSphere(cone));
				
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
	StructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(SpotLightProps), true, false);
	StructuredBufferDesc lightFrustumBufferDesc(m_NumLights, sizeof(LightFrustum), true, false);
	StructuredBufferDesc lightViewProjMatrixBufferDesc(m_NumLights, sizeof(Matrix4f), true, false);

	m_pLightBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");
	m_pLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightFrustumBuffer");
	m_pLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightViewProjMatrixBuffer");

	m_pUploadLightBoundsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(SpotLightProps));

	m_pUploadLightFrustumBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightFrustumBuffer");
	m_pUploadLightFrustumBuffer->Write(lightFrustums.data(), m_NumLights * sizeof(LightFrustum));

	m_pUploadLightViewProjMatrixBuffer = new Buffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightViewProjMatrixBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightViewProjMatrixBuffer");
	m_pUploadLightViewProjMatrixBuffer->Write(lightViewProjMatrices.data(), m_NumLights * sizeof(Matrix4f));
}

LightBuffer::~LightBuffer()
{
	RemoveDataForUpload();

	SafeDelete(m_pLightBoundsBuffer);
	SafeDelete(m_pLightPropsBuffer);
	SafeDelete(m_pLightFrustumBuffer);
	SafeDelete(m_pLightViewProjMatrixBuffer);
}

void LightBuffer::RecordDataForUpload(CommandList* pCommandList)
{
	pCommandList->CopyResource(m_pLightBoundsBuffer, m_pUploadLightBoundsBuffer);
	pCommandList->CopyResource(m_pLightPropsBuffer, m_pUploadLightPropsBuffer);
	pCommandList->CopyResource(m_pLightFrustumBuffer, m_pUploadLightFrustumBuffer);
	pCommandList->CopyResource(m_pLightViewProjMatrixBuffer, m_pUploadLightViewProjMatrixBuffer);

	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		ResourceTransitionBarrier(m_pLightBoundsBuffer, m_pLightBoundsBuffer->GetState(), m_pLightBoundsBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pLightPropsBuffer, m_pLightPropsBuffer->GetState(), m_pLightPropsBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pLightFrustumBuffer, m_pLightFrustumBuffer->GetState(), m_pLightFrustumBuffer->GetReadState()),
		ResourceTransitionBarrier(m_pLightViewProjMatrixBuffer, m_pLightViewProjMatrixBuffer->GetState(), m_pLightViewProjMatrixBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);

	m_pLightBoundsBuffer->SetState(m_pLightBoundsBuffer->GetReadState());
	m_pLightPropsBuffer->SetState(m_pLightPropsBuffer->GetReadState());
	m_pLightFrustumBuffer->SetState(m_pLightFrustumBuffer->GetReadState());
	m_pLightViewProjMatrixBuffer->SetState(m_pLightViewProjMatrixBuffer->GetReadState());
}

void LightBuffer::RemoveDataForUpload()
{
	SafeDelete(m_pUploadLightBoundsBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
	SafeDelete(m_pUploadLightFrustumBuffer);
	SafeDelete(m_pUploadLightViewProjMatrixBuffer);
}