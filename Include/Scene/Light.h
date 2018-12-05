#pragma once

#include "Math/BasisAxes.h"

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

class PointLight
{
public:
	PointLight(const Vector3f& worldPosition, const Vector3f& radiantPower, f32 range, f32 shadowNearPlane, f32 expShadowMapConstant);

	const Vector3f& GetWorldPosition() const;
	void SetWorldPosition(const Vector3f& worldPosition);

	const Vector3f& GetRadiantPower() const;
	void SetRadiantPower(const Vector3f& radiantPower);
	Vector3f EvaluateRadiantIntensity() const;

	f32 GetRange() const;
	void SetRange(f32 range);

	f32 GetShadowNearPlane() const;
	void SetShadowNearPlane(float shadowNearPlane);

	f32 GetExpShadowMapConstant() const;
	void SetExpShadowMapConstant(f32 expShadowMapConstant);

private:
	Vector3f m_WorldPosition;
	Vector3f m_RadiantPower;
	f32 m_Range;
	f32 m_ShadowNearPlane;
	f32 m_ExpShadowMapConstant;
};

class SpotLight
{
public:
	// innerConeAngle <= outerConeAngle
	// outerConeAngle <= 180 degrees

	SpotLight(const Vector3f& worldPosition, const BasisAxes& worldOrientation, const Vector3f& radiantPower, f32 range,
		f32 innerConeAngleInRadians, f32 outerConeAngleInRadians, f32 shadowNearPlane, f32 expShadowMapConstant);

	const Vector3f& GetWorldPosition() const;
	void SetWorldPosition(const Vector3f& worldPosition);

	const BasisAxes& GetWorldOrientation() const;
	void SetWorldOrientation(const BasisAxes& worldOrientation);

	const Vector3f& GetRadiantPower() const;
	void SetRadiantPower(const Vector3f& radiantPower);
	Vector3f EvaluateRadiantIntensity() const;

	f32 GetRange() const;
	void SetRange(f32 range);
	
	f32 GetInnerConeAngle() const;
	void SetInnerConeAngle(f32 innerConeAngleInRadians);

	f32 GetOuterConeAngle() const;
	void SetOuterConeAngle(f32 outerConeAngleInRadians);

	f32 GetShadowNearPlane() const;
	void SetShadowNearPlane(f32 shadowNearPlane);

	f32 GetExpShadowMapConstant() const;
	void SetExpShadowMapConstant(f32 expShadowMapConstant);

private:
	Vector3f m_WorldPosition;
	BasisAxes m_WorldOrientation;
	Vector3f m_RadiantPower;
	f32 m_Range;
	f32 m_InnerConeAngleInRadians;
	f32 m_OuterConeAngleInRadians;
	f32 m_ShadowNearPlane;
	f32 m_ExpShadowMapConstant;
};

struct DirectionalLight
{
	DirectionalLight(const Vector3f& worldDirection, const Vector3f& irradiancePerpToLightDirection);

	Vector3f m_WorldDirection;
	Vector3f m_IrradiancePerpToLightDirection;
};