#include "Common/LightBuffer.h"
#include "Common/Light.h"
#include "DX/DXResource.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXCommandList.h"
#include "Math/Cone.h"
#include "Math/Transform.h"
#include "Math/Quaternion.h"

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

LightBuffer::~LightBuffer()
{
	SafeDelete(m_pUploadLightBoundsBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
	SafeDelete(m_pLightBoundsBuffer);
	SafeDelete(m_pLightPropsBuffer);
}

LightBuffer::LightBuffer(DXRenderEnvironment* pEnv, u32 numPointLights, PointLight** ppPointLights)
	: m_NumLights(numPointLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<PointLightProps> lightProps;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);

	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];
				
		lightBounds.emplace_back(pLight->GetTransform().GetPosition(), pLight->GetAttenEndRange());
		lightProps.emplace_back(pLight->GetColor(), pLight->GetAttenStartRange());
	}

	DXStructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(PointLightProps), true, false);

	m_pLightBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");

	m_pUploadLightBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(PointLightProps));
}

LightBuffer::LightBuffer(DXRenderEnvironment* pEnv, u32 numSpotLights, SpotLight** ppSpotLights)
	: m_NumLights(numSpotLights)
	, m_pUploadLightBoundsBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pLightBoundsBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
{
	std::vector<Sphere> lightBounds;
	std::vector<SpotLightProps> lightProps;

	lightBounds.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);

	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const SpotLight* pSpotLight = ppSpotLights[lightIndex];

		const Transform& lightTransform = pSpotLight->GetTransform();
		const Vector3f& lightPos = lightTransform.GetPosition();

		const BasisAxes lightBasis = ExtractBasisAxes(lightTransform.GetRotation());
		const Vector3f lightDir = Normalize(lightBasis.m_ZAxis);

		Cone cone(lightPos, pSpotLight->GetOuterConeAngle(), lightDir, pSpotLight->GetAttenEndRange());
		lightBounds.emplace_back(ExtractBoundingSphere(cone));
				
		f32 cosHalfInnerConeAngle = Cos(0.5f * pSpotLight->GetInnerConeAngle());
		f32 cosHalfOuterConeAngle = Cos(0.5f * pSpotLight->GetOuterConeAngle());

		lightProps.emplace_back(pSpotLight->GetColor(), lightDir, pSpotLight->GetAttenStartRange(), pSpotLight->GetAttenEndRange(), cosHalfInnerConeAngle, cosHalfOuterConeAngle);
	}

	DXStructuredBufferDesc lightBoundsBufferDesc(m_NumLights, sizeof(Sphere), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(SpotLightProps), true, false);

	m_pLightBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightBoundsBuffer");
	m_pLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");

	m_pUploadLightBoundsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightBoundsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightBoundsBuffer");
	m_pUploadLightBoundsBuffer->Write(lightBounds.data(), m_NumLights * sizeof(Sphere));

	m_pUploadLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(SpotLightProps));
}

void LightBuffer::RecordDataForUpload(DXCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pLightBoundsBuffer, m_pUploadLightBoundsBuffer);
	pCommandList->CopyResource(m_pLightPropsBuffer, m_pUploadLightPropsBuffer);

	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		DXResourceTransitionBarrier(m_pLightBoundsBuffer, m_pLightBoundsBuffer->GetState(), m_pLightBoundsBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pLightPropsBuffer, m_pLightPropsBuffer->GetState(), m_pLightPropsBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);

	m_pLightBoundsBuffer->SetState(m_pLightBoundsBuffer->GetReadState());
	m_pLightPropsBuffer->SetState(m_pLightPropsBuffer->GetReadState());
}

void LightBuffer::RemoveDataForUpload()
{
	SafeDelete(m_pUploadLightBoundsBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
}