#pragma once

#include "Scene/SceneObject.h"
#include "Math/Vector4.h"

enum LightType
{
	LightType_Spot = 1,
	LightType_Directional = 2,
	LightType_Point = 3
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
	f32 GetIntensity() const;
	void SetIntensity(f32 inensity);

	const Vector3f& GetColor() const;
	void SetColor(const Vector3f& color);
				
private:
	f32 m_Intensity;
	Vector3f m_Color;
};

class PointLight : public Light
{
public:
	PointLight(const std::string& name, f32 range, f32 shadowNearPlane);

	f32 GetRange() const;
	void SetRange(f32 range);
	
	f32 GetShadowNearPlane() const;
	void SetShadowNearPlane(f32 shadowNearPlane);

private:
	f32 m_Range;
	f32 m_ShadowNearPlane;
};

class SpotLight : public Light
{
public:
	SpotLight(const std::string& name, f32 range, f32 innerConeAngleInRadians, f32 outerConeAngleInRadians, f32 shadowNearPlane);

	f32 GetRange() const;
	void SetRange(f32 range);
		
	f32 GetInnerConeAngle() const;
	void SetInnerConeAngle(f32 innerConeAngleInRadians);

	f32 GetOuterConeAngle() const;
	void SetOuterConeAngle(f32 outerConeAngleInRadians);

	f32 GetShadowNearPlane() const;
	void SetShadowNearPlane(f32 shadowNearPlane);

private:
	f32 m_Range;
	f32 m_InnerConeAngleInRadians;
	f32 m_OuterConeAngleInRadians;
	f32 m_ShadowNearPlane;
};

class DirectionalLight : public Light
{
public:
	DirectionalLight(const std::string& name);
};