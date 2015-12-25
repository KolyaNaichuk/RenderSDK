#include "Math/Radian.h"
#include "Math/Degree.h"

Radian::Radian(f32 radian)
	: m_Radian(radian)
{
}

Radian::Radian(const Radian& radian)
	: m_Radian(radian.m_Radian)
{
}

Radian::Radian(const Degree& degree)
	: m_Radian(DegreesToRadians(degree.Get()))
{
}

f32 Radian::Get() const
{
	return m_Radian;
}

Radian& Radian::operator= (const Radian& radian)
{
	m_Radian = radian.m_Radian;
	return *this;
}

Radian& Radian::operator= (const Degree& degree)
{
	m_Radian = DegreesToRadians(degree.Get());
	return *this;
}

Radian& Radian::operator+= (const Radian& radian)
{
	m_Radian += radian.m_Radian;
	return *this;
}

Radian& Radian::operator+= (const Degree& degree)
{
	m_Radian += DegreesToRadians(degree.Get());
	return *this;
}

const Radian Radian::operator+ (const Radian& radian)
{
	return Radian(m_Radian + radian.m_Radian);
}

const Radian Radian::operator+ (const Degree& degree)
{
	return Radian(m_Radian + DegreesToRadians(degree.Get()));
}

Radian& Radian::operator-= (const Radian& radian)
{
	m_Radian -= radian.m_Radian;
	return *this;
}

Radian& Radian::operator-= (const Degree& degree)
{
	m_Radian -= DegreesToRadians(degree.Get());
	return *this;
}

const Radian Radian::operator- (const Radian& radian)
{
	return Radian(m_Radian - radian.m_Radian);
}

const Radian Radian::operator- (const Degree& degree)
{
	return Radian(m_Radian - DegreesToRadians(degree.Get()));
}

Radian& Radian::operator*= (const Radian& radian)
{
	m_Radian *= radian.m_Radian;
	return *this;
}

Radian& Radian::operator*= (const Degree& degree)
{
	m_Radian *= DegreesToRadians(degree.Get());
	return *this;
}

const Radian Radian::operator* (const Radian& radian)
{
	return Radian(m_Radian * radian.m_Radian);
}

const Radian Radian::operator* (const Degree& degree)
{
	return Radian(m_Radian * DegreesToRadians(degree.Get()));
}

Radian& Radian::operator/= (const Radian& radian)
{
	m_Radian /= radian.m_Radian;
	return *this;
}

Radian& Radian::operator/= (const Degree& degree)
{
	m_Radian /= DegreesToRadians(degree.Get());
	return *this;
}

const Radian Radian::operator/ (const Radian& radian)
{
	return Radian(m_Radian / radian.m_Radian);
}

const Radian Radian::operator/ (const Degree& degree)
{
	return Radian(m_Radian / DegreesToRadians(degree.Get()));
}
