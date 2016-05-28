#include "Common/LightBuffer.h"
#include "Common/Light.h"
#include "DX/DXResource.h"
#include "DX/DXRenderEnvironment.h"
#include "DX/DXCommandList.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

struct PointLightGeometry
{
	PointLightGeometry(const Vector3f& worldSpacePos, f32 attenEndRange)
		: m_WorldSpacePos(worldSpacePos)
		, m_AttenEndRange(attenEndRange)
	{}
	Vector3f m_WorldSpacePos;
	f32 m_AttenEndRange;
};

struct PointLightProps
{
	PointLightProps(const Vector3f& color, f32 attenStartRange)
		: m_Color(color)
		, m_AttenStartRange(attenStartRange)
	{}
	Vector3f m_Color;
	f32 m_AttenStartRange;
};

struct SpotLightGeometry
{
	SpotLightGeometry(const Vector3f& worldSpacePos, const Vector3f& worldSpaceDir, f32 attenEndRange)
		: m_WorldSpacePos(worldSpacePos)
		, m_WorldSpaceDir(worldSpaceDir)
		, m_AttenEndRange(attenEndRange)
	{}
	Vector3f m_WorldSpacePos;
	Vector3f m_WorldSpaceDir;
	f32 m_AttenEndRange;
};

struct SpotLightProps
{
	SpotLightProps(const Vector3f& color, f32 attenStartRange, f32 cosHalfInnerConeAngle, f32 cosHalfOuterConeAngle)
		: m_Color(color)
		, m_AttenStartRange(attenStartRange)
		, m_CosHalfInnerConeAngle(cosHalfInnerConeAngle)
		, m_CosHalfOuterConeAngle(cosHalfOuterConeAngle)
	{}
	Vector3f m_Color;
	f32 m_AttenStartRange;
	f32 m_CosHalfInnerConeAngle;
	f32 m_CosHalfOuterConeAngle;
};

LightBuffer::~LightBuffer()
{
	SafeDelete(m_pUploadLightGeometryBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
	SafeDelete(m_pLightGeometryBuffer);
	SafeDelete(m_pLightPropsBuffer);
}

LightBuffer::LightBuffer(DXRenderEnvironment* pEnv, u32 numPointLights, PointLight** ppPointLights)
	: m_NumLights(numPointLights)
	, m_pUploadLightGeometryBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pLightGeometryBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
{
	std::vector<PointLightGeometry> lightGeometry;
	std::vector<PointLightProps> lightProps;

	lightGeometry.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);

	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const PointLight* pLight = ppPointLights[lightIndex];
		const Transform& lightTransform = pLight->GetTransform();

		lightGeometry.emplace_back(lightTransform.GetPosition(), pLight->GetAttenEndRange());
		lightProps.emplace_back(pLight->GetColor(), pLight->GetAttenStartRange());
	}

	DXStructuredBufferDesc lightGeometryBufferDesc(m_NumLights, sizeof(PointLightGeometry), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(PointLightProps), true, false);

	m_pLightGeometryBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightGeometryBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightGeometryBuffer");
	m_pLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");

	m_pUploadLightGeometryBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightGeometryBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightGeometryBuffer");
	m_pUploadLightGeometryBuffer->Write(lightGeometry.data(), m_NumLights * sizeof(PointLightGeometry));

	m_pUploadLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(PointLightProps));
}

LightBuffer::LightBuffer(DXRenderEnvironment* pEnv, u32 numSpotLights, SpotLight** ppSpotLights)
	: m_NumLights(numSpotLights)
	, m_pUploadLightGeometryBuffer(nullptr)
	, m_pUploadLightPropsBuffer(nullptr)
	, m_pLightGeometryBuffer(nullptr)
	, m_pLightPropsBuffer(nullptr)
{
	std::vector<SpotLightGeometry> lightGeometry;
	std::vector<SpotLightProps> lightProps;

	lightGeometry.reserve(m_NumLights);
	lightProps.reserve(m_NumLights);

	for (u32 lightIndex = 0; lightIndex < m_NumLights; ++lightIndex)
	{
		const SpotLight* pSpotLight = ppSpotLights[lightIndex];

		const Transform& lightTransform = pSpotLight->GetTransform();
		const Vector3f& lightPos = lightTransform.GetPosition();
		const Vector3f lightDir = ExtractBasisAxes(lightTransform.GetRotation()).m_ZAxis;

		lightGeometry.emplace_back(lightPos, lightDir, pSpotLight->GetAttenEndRange());

		f32 cosHalfInnerConeAngle = Cos(0.5f * pSpotLight->GetInnerConeAngle());
		f32 cosHalfOuterConeAngle = Cos(0.5f * pSpotLight->GetOuterConeAngle());

		lightProps.emplace_back(pSpotLight->GetColor(), pSpotLight->GetAttenStartRange(), cosHalfInnerConeAngle, cosHalfOuterConeAngle);
	}

	DXStructuredBufferDesc lightGeometryBufferDesc(m_NumLights, sizeof(SpotLightGeometry), true, false);
	DXStructuredBufferDesc lightPropsBufferDesc(m_NumLights, sizeof(SpotLightProps), true, false);

	m_pLightGeometryBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightGeometryBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightGeometryBuffer");
	m_pLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pDefaultHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, L"LightBuffer::m_pLightPropsBuffer");

	m_pUploadLightGeometryBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightGeometryBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightGeometryBuffer");
	m_pUploadLightGeometryBuffer->Write(lightGeometry.data(), m_NumLights * sizeof(SpotLightGeometry));

	m_pUploadLightPropsBuffer = new DXBuffer(pEnv, pEnv->m_pUploadHeapProps, &lightPropsBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, L"LightBuffer::m_pUploadLightPropsBuffer");
	m_pUploadLightPropsBuffer->Write(lightProps.data(), m_NumLights * sizeof(SpotLightProps));
}

void LightBuffer::RecordDataForUpload(DXCommandList* pCommandList)
{
	pCommandList->CopyResource(m_pLightGeometryBuffer, m_pUploadLightGeometryBuffer);
	pCommandList->CopyResource(m_pLightPropsBuffer, m_pUploadLightPropsBuffer);

	const D3D12_RESOURCE_BARRIER resourceTransitions[] =
	{
		DXResourceTransitionBarrier(m_pLightGeometryBuffer, m_pLightGeometryBuffer->GetState(), m_pLightGeometryBuffer->GetReadState()),
		DXResourceTransitionBarrier(m_pLightPropsBuffer, m_pLightPropsBuffer->GetState(), m_pLightPropsBuffer->GetReadState())
	};
	pCommandList->ResourceBarrier(ARRAYSIZE(resourceTransitions), &resourceTransitions[0]);

	m_pLightGeometryBuffer->SetState(m_pLightGeometryBuffer->GetReadState());
	m_pLightPropsBuffer->SetState(m_pLightPropsBuffer->GetReadState());
}

void LightBuffer::RemoveDataForUpload()
{
	SafeDelete(m_pUploadLightGeometryBuffer);
	SafeDelete(m_pUploadLightPropsBuffer);
}