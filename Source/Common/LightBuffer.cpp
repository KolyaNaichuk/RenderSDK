#include "Common/LightBuffer.h"
#include "Common/Light.h"
#include "DX/DXResource.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXCommandList.h"
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

LightBuffer::LightBuffer(DXRenderEnvironment* pRenderEnv, u32 numPointLights, PointLight** ppPointLights)
	: m_NumLights(numPointLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pUploadLightFrustumBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<PointLightProps> lightProps;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);

	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();
				
		lightBounds.emplace_back(lightWorldSpacePos, pLight->GetAttenEndRange());
		lightProps.emplace_back(pLight->GetColor(), pLight->GetAttenStartRange());
	}

	DXStructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(PointLightProps), true, false);

	m_pLightBoundsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");

	m_pUploadLightBoundsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(PointLightProps));
}

LightBuffer::LightBuffer(DXRenderEnvironment* pRenderEnv, u32 numSpotLights, SpotLight** ppSpotLights)
	: m_NumLights(numSpotLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pUploadLightFrustumBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
	, m_pLightFrustumBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<SpotLightProps> lightProps;
	std::vector<LightFrustum> lightFrustums;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);
	lightFrustums.reserve(m_NumLights);
		
	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const SpotLight* pLight = ppSpotLights[lightIndex];

		const Transform& lightWorldSpaceTransform = pLight->GetTransform();
		const Vector3f& lightWorldSpacePos = lightWorldSpaceTransform.GetPosition();

		const BasisAxes lightWorldSpaceBasis = ExtractBasisAxes(lightWorldSpaceTransform.GetRotation());
		const Vector3f lightWorldSpaceDir = Normalize(lightWorldSpaceBasis.m_ZAxis);

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
	}

	DXStructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(SpotLightProps), true, false);
	DXStructuredBufferDesc lightFrustumBufferDesc(m_NumLights, sizeof(LightFrustum), true, false);

	m_pLightBoundsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");
	m_pLightFrustumBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightFrustumBuffer");

	m_pUploadLightBoundsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(SpotLightProps));

	m_pUploadLightFrustumBuffer = new DXBuffer(pRenderEnv, pRenderEnv->m_pDefaultHeapProps, &lightFrustumBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightFrustumBuffer");
	m_pUploadLightFrustumBuffer->Write(lightFrustums.data(), m_NumLights * sizeof(LightFrustum));
}

LightBuffer::~LightBuffer()
{
	RemoveDataForUpload();

	SafeDelete(m_pLightBoundsBuffer);
	SafeDelete(m_pLightPropsBuffer);
	SafeDelete(m_pLightFrustumBuffer);
}

void LightBuffer::RecordDataForUpload(DXCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pLightBoundsBuffer, m_pUploadLightBoundsBuffer);
	pCommandList->CopyResource(m_pLightPropsBuffer, m_pUploadLightPropsBuffer);
	pCommandList->CopyResource(m_pLightFrustumBuffer, m_pUploadLightFrustumBuffer);

	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		DXResourceTransitionBarrier(m_pLightBoundsBuffer, m_pLightBoundsBuffer->GetState(), m_pLightBoundsBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pLightPropsBuffer, m_pLightPropsBuffer->GetState(), m_pLightPropsBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pLightFrustumBuffer, m_pLightFrustumBuffer->GetState(), m_pLightFrustumBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);

	m_pLightBoundsBuffer->SetState(m_pLightBoundsBuffer->GetReadState());
	m_pLightPropsBuffer->SetState(m_pLightPropsBuffer->GetReadState());
	m_pLightFrustumBuffer->SetState(m_pLightFrustumBuffer->GetReadState());
}

void LightBuffer::RemoveDataForUpload()
{
	SafeDelete(m_pUploadLightBoundsBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
	SafeDelete(m_pUploadLightFrustumBuffer);
}