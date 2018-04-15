#include "Math/Sphere.h"
#include "Math/Math.h"

Sphere::Sphere()
	: m_Center(0.0f, 0.0f, 0.0f)
	, m_Radius(0.0f)
{
}

Sphere::Sphere(const Vector3f& center, f32 radius)
    : m_Center(center)
    , m_Radius(radius)
{
}

Sphere::Sphere(u32 numPoints, const Vector3f* pFirstPoint)
{
	assert(false);
}

Sphere::Sphere(const Sphere& sphere1, const Sphere& sphere2)
{
    Vector3f centerDir = sphere2.m_Center - sphere1.m_Center;
    f32 sqLength = LengthSquared(centerDir);
    if (Sqr(sphere2.m_Radius - sphere1.m_Radius) > sqLength)
    {
        f32 length = Sqrt(sqLength);
        m_Radius = sphere1.m_Radius + length + sphere2.m_Radius;
        m_Center = sphere1.m_Center + centerDir * ((m_Radius - sphere1.m_Radius) / length);    
    }
    else
    {
        const Sphere& enclosingSphere = (sphere2.m_Radius > sphere1.m_Radius) ? sphere2 : sphere1;
        m_Center = enclosingSphere.m_Center;
        m_Radius = enclosingSphere.m_Radius;
    }
}
