#pragma once

#include "Common/SceneObject.h"
#include "Math/Vector4.h"
#include "Math/Radian.h"

enum LightType
{
	LightType_Point = 1,
	LightType_Spot = 2,
	LightType_Directional = 3
};

enum CubeMapFaces
{
	kCubeMapFacePosX = 0,
	kCubeMapFaceNegX,
	kCubeMapFacePosY,
	kCubeMapFaceNegY,
	kCubeMapFacePosZ,
	kCubeMapFaceNegZ,
	kNumCubeMapFaces
};

class Light : public SceneObject
{
protected:
	Light(const std::string& name);

public:
	bool AffectsWorld() const;
	void SetAffectsWorld(bool affectsWorld);

	f32 GetIntensity() const;
	void SetIntensity(f32 inensity);

	const Vector3f& GetColor() const;
	void SetColor(const Vector3f& color);

	bool CastsShadows() const;
	void SetCastsShadows(bool castsShadows);

	f32 GetShadowBias() const;
	void SetShadowBias(f32 shadowBias);
			
private:
	bool m_AffectsWorld;
	f32 m_Intensity;
	Vector3f m_Color;
	bool m_CastsShadows;
	f32 m_ShadowBias;
};

class PointLight : public Light
{
public:
	PointLight(const std::string& name, f32 range);

	f32 GetRange() const;
	void SetRange(f32 range);

	f32 GetAttenStartRange() const;
	void SetAttenStartRange(f32 attenStartRange);

	f32 GetAttenEndRange() const;
	void SetAttenEndRange(f32 attenEndRange);

private:
	f32 m_Range;
	f32 m_AttenStartRange;
	f32 m_AttenEndRange;
};

class SpotLight : public Light
{
public:
	SpotLight(const std::string& name, f32 range, const Radian& innerConeAngle, const Radian& outerConeAngle);

	f32 GetRange() const;
	void SetRange(f32 range);

	f32 GetAttenStartRange() const;
	void SetAttenStartRange(f32 attenStartRange);

	f32 GetAttenEndRange() const;
	void SetAttenEndRange(f32 attenEndRange);
	
	const Radian& GetInnerConeAngle() const;
	void SetInnerConeAngle(const Radian& innerConeAngle);

	const Radian& GetOuterConeAngle() const;
	void SetOuterConeAngle(const Radian& outerConeAngle);

private:
	f32 m_Range;
	f32 m_AttenStartRange;
	f32 m_AttenEndRange;
	Radian m_InnerConeAngle;
	Radian m_OuterConeAngle;
};

class DirectionalLight : public Light
{
public:
	DirectionalLight(const std::string& name);
};