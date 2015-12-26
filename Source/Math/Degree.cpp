#include "Math/Degree.h"
#include "Math/Radian.h"

Degree::Degree(f32 degree)
	: m_Degree(degree)
{
}

Degree::Degree(const Degree& degree)
	: m_Degree(degree.m_Degree)
{
}

Degree::Degree(const Radian& radian)
	: m_Degree(RadiansToDegrees(radian.Get()))
{
}

f32 Degree::Get() const
{
	return m_Degree;
}

Degree& Degree::operator= (f32 degree)
{
	m_Degree = degree;
	return *this;
}

Degree& Degree::operator= (const Degree& degree)
{
	m_Degree = degree.m_Degree;
	return *this;
}

Degree& Degree::operator= (const Radian& radian)
{
	m_Degree = RadiansToDegrees(radian.Get());
	return *this;
}

Degree& Degree::operator+= (f32 degree)
{
	m_Degree += degree;
	return *this;
}

Degree& Degree::operator+= (const Degree& degree)
{
	m_Degree += degree.m_Degree;
	return *this;
}

Degree& Degree::operator+= (const Radian& radian)
{
	m_Degree += RadiansToDegrees(radian.Get());
	return *this;
}

const Degree Degree::operator+ (const Degree& degree)
{
	return Degree(m_Degree + degree.m_Degree);
}

const Degree Degree::operator+ (const Radian& radian)
{
	return Degree(m_Degree + RadiansToDegrees(radian.Get()));
}

Degree& Degree::operator-= (f32 degree)
{
	m_Degree -= degree;
	return *this;
}

Degree& Degree::operator-= (const Degree& degree)
{
	m_Degree -= degree.m_Degree;
	return *this;
}

Degree& Degree::operator-= (const Radian& radian)
{
	m_Degree -= RadiansToDegrees(radian.Get());
	return *this;
}

const Degree Degree::operator- (const Degree& degree)
{
	return Degree(m_Degree - degree.m_Degree);
}

const Degree Degree::operator- (const Radian& radian)
{
	return Degree(m_Degree - RadiansToDegrees(radian.Get()));
}

Degree& Degree::operator*= (f32 degree)
{
	m_Degree *= degree;
	return *this;
}

Degree& Degree::operator*= (const Degree& degree)
{
	m_Degree *= degree.m_Degree;
	return *this;
}

Degree& Degree::operator*= (const Radian& radian)
{
	m_Degree *= RadiansToDegrees(radian.Get());
	return *this;
}

const Degree Degree::operator* (const Degree& degree)
{
	return Degree(m_Degree * degree.m_Degree);
}

const Degree Degree::operator* (const Radian& radian)
{
	return Degree(m_Degree * RadiansToDegrees(radian.Get()));
}

Degree& Degree::operator/= (f32 degree)
{
	m_Degree /= degree;
	return *this;
}

Degree& Degree::operator/= (const Degree& degree)
{
	m_Degree /= degree.m_Degree;
	return *this;
}

Degree& Degree::operator/= (const Radian& radian)
{
	m_Degree /= RadiansToDegrees(radian.Get());
	return *this;
}

const Degree Degree::operator/ (const Degree& degree)
{
	return Degree(m_Degree / degree.m_Degree);
}

const Degree Degree::operator/ (const Radian& radian)
{
	return Degree(m_Degree / RadiansToDegrees(radian.Get()));
}

const Degree operator+ (f32 left, const Degree& right)
{
	return Degree(left + right.Get());
}

const Degree operator+ (const Degree& left, f32 right)
{
	return Degree(left.Get() + right);
}

const Degree operator- (f32 left, const Degree& right)
{
	return Degree(left - right.Get());
}

const Degree operator- (const Degree& left, f32 right)
{
	return Degree(left.Get() - right);
}

const Degree operator* (f32 left, const Degree& right)
{
	return Degree(left * right.Get());
}

const Degree operator* (const Degree& left, f32 right)
{
	return Degree(left.Get() * right);
}

const Degree operator/ (f32 left, const Degree& right)
{
	return Degree(left / right.Get());
}

const Degree operator/ (const Degree& left, f32 right)
{
	return Degree(left.Get() / right);
}
