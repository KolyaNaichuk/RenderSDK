#include "Math/Transform.h"
#include "Math/Vector3f.h"
#include "Math/Math.h"

namespace Transform
{
    const Matrix4f Translation(f32 xOffset, f32 yOffset, f32 zOffset)
    {
        return Matrix4f(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        xOffset, yOffset, zOffset, 1.0f);
    }
    
    const Matrix4f Scaling(f32 xScale, f32 yScale, f32 zScale)
    {
        return Matrix4f(xScale, 0.0f, 0.0f, 0.0f,
                        0.0f, yScale, 0.0f, 0.0f,
                        0.0f, 0.0f, zScale, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    const Matrix4f RotationX(f32 angleInRadians)
    {
        f32 sinAngle = Sin(angleInRadians);
        f32 cosAngle = Cos(angleInRadians);

        return Matrix4f(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, cosAngle, sinAngle, 0.0f,
                        0.0f, -sinAngle, cosAngle, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);  
    }
    
    const Matrix4f RotationY(f32 angleInRadians)
    {
        f32 sinAngle = Sin(angleInRadians);
        f32 cosAngle = Cos(angleInRadians);
        
        return Matrix4f(cosAngle, 0.0f, -sinAngle, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        sinAngle, 0.0f, cosAngle, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);  
    }
    
    const Matrix4f RotationZ(f32 angleInRadians)
    {
        f32 sinAngle = Sin(angleInRadians);
        f32 cosAngle = Cos(angleInRadians);

        return Matrix4f(cosAngle, sinAngle, 0.0f, 0.0f,
                       -sinAngle, cosAngle, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    const Matrix4f LookAt(const Vector3f& eyePos, const Vector3f& lookAtPos, const Vector3f& upDir)
    {
        Vector3f zAxis = Normalize(lookAtPos - eyePos);
        Vector3f xAxis = Normalize(Cross(upDir, zAxis));
        Vector3f yAxis = Cross(zAxis, xAxis);

        return Matrix4f(xAxis.m_X, yAxis.m_X, zAxis.m_X, 0.0f,
                        xAxis.m_Y, yAxis.m_Y, zAxis.m_Y, 0.0f,
                        xAxis.m_Z, yAxis.m_Z, zAxis.m_Z, 0.0f,
                       -Dot(xAxis, eyePos), -Dot(yAxis, eyePos), -Dot(zAxis, eyePos), 1.0f);
    }

	const Matrix4f OrthoOffCenterProj(f32 leftX, f32 rightX, f32 bottomY, f32 topY, f32 nearZ, f32 farZ)
	{
		return Matrix4f(2.0f / (rightX - leftX), 0.0f, 0.0f, 0.0f,
						0.0f, 2.0f / (topY - bottomY), 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f / (farZ - nearZ), 0.0f,
					   (leftX + rightX) / (leftX - rightX), (topY + bottomY) / (bottomY - topY), nearZ / (nearZ / farZ), 1.0f);
	}

    const Matrix4f OrthoProj(f32 width, f32 height, f32 nearZ, f32 farZ)
    {
        return Matrix4f(2.0f / width, 0.0f, 0.0f, 0.0f,
			            0.0f, 2.0f / height, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f / (farZ - nearZ), 0.0f,
						0.0f, 0.0f, -nearZ / (farZ - nearZ), 1.0f);
    }
    
    const Matrix4f PerspectiveProj(f32 nearWidth, f32 nearHeight, f32 nearZ, f32 farZ)
    {
        return Matrix4f(2.0f * nearZ / nearWidth, 0.0f, 0.0f, 0.0f,
                        0.0f, 2 * nearZ / nearHeight, 0.0f, 0.0f,
                        0.0f, 0.0f, farZ / (farZ - nearZ), 1.0f,
                        0.0f, 0.0f, nearZ * farZ / (nearZ - farZ), 0.0f);
    }

    const Matrix4f PerspectiveFovProj(f32 fovYInRadians, f32 aspectRatio, f32 nearZ, f32 farZ)
    {
        f32 yScale = Rcp(Tan(0.5f * fovYInRadians));
        f32 xScale = yScale / aspectRatio;

        return Matrix4f(xScale, 0.0f, 0.0f, 0.0f,
                        0.0f, yScale, 0.0f, 0.0f,
                        0.0f, 0.0f, farZ / (farZ - nearZ), 1.0f,
                        0.0f, 0.0f, nearZ * farZ / (nearZ - farZ), 0.0f);  
    }

    const Vector4f TransformVector(const Vector4f& vec, const Matrix4f& matrix)
    {
        return Vector4f(vec.m_X * matrix.m_00 + vec.m_Y * matrix.m_10 + vec.m_Z * matrix.m_20 + vec.m_W * matrix.m_30,
                        vec.m_X * matrix.m_01 + vec.m_Y * matrix.m_11 + vec.m_Z * matrix.m_21 + vec.m_W * matrix.m_31,
                        vec.m_X * matrix.m_02 + vec.m_Y * matrix.m_12 + vec.m_Z * matrix.m_22 + vec.m_W * matrix.m_32,
                        vec.m_X * matrix.m_03 + vec.m_Y * matrix.m_13 + vec.m_Z * matrix.m_23 + vec.m_W * matrix.m_33);
    }
        
    const Vector4f TransformNormal(const Vector4f& vec, const Matrix4f& matrix)
    {
        return TransformVector(vec, Transpose(Inverse(matrix)));
    }
}