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

Radian& Radian::operator= (f32 radian)
{
	m_Radian = radian;
	return *this;
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

Radian& Radian::operator+= (f32 radian)
{
	m_Radian += radian;
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

Radian& Radian::operator-= (f32 radian)
{
	m_Radian -= radian;
	return *this;
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

Radian& Radian::operator*= (f32 radian)
{
	m_Radian *= radian;
	return *this;
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

Radian& Radian::operator/= (f32 radian)
{
	m_Radian /= radian;
	return *this;
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

const Radian Radian::operator- () const
{
	return Radian(-m_Radian);
}

const Radian operator+ (f32 left, const Radian& right)
{
	return Radian(left + right.Get());
}

const Radian operator+ (const Radian& left, f32 right)
{
	return Radian(left.Get() + right);
}

const Radian operator- (f32 left, const Radian& right)
{
	return Radian(left - right.Get());
}

const Radian operator- (const Radian& left, f32 right)
{
	return Radian(left.Get() - right);
}

const Radian operator* (f32 left, const Radian& right)
{
	return Radian(left * right.Get());
}

const Radian operator* (const Radian& left, f32 right)
{
	return Radian(left.Get() * right);
}

const Radian operator/ (f32 left, const Radian& right)
{
	return Radian(left / right.Get());
}

const Radian operator/ (const Radian& left, f32 right)
{
	return Radian(left.Get() / right);
}