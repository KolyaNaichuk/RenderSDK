#include "Math/BoundingSphere.h"
#include "Math/Math.h"

BoundingSphere::BoundingSphere(const Vector3f& center, f32 radius)
    : m_Center(center)
    , m_Radius(radius)
{
}

BoundingSphere::BoundingSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
{
    Vector3f centerDir = sphere2.m_Center - sphere1.m_Center;
    f32 sqLength = LengthSq(centerDir);
    if (Sqr(sphere2.m_Radius - sphere1.m_Radius) > sqLength)
    {
        f32 length = Sqrt(sqLength);
        m_Radius = sphere1.m_Radius + length + sphere2.m_Radius;
        m_Center = sphere1.m_Center + centerDir * ((m_Radius - sphere1.m_Radius) / length);    
    }
    else
    {
        const BoundingSphere& enclosingSphere = (sphere2.m_Radius > sphere1.m_Radius) ? sphere2 : sphere1;
        m_Center = enclosingSphere.m_Center;
        m_Radius = enclosingSphere.m_Radius;
    }
}
