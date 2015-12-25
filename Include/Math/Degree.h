#pragma once

#include "Math/Math.h"

class Radian;

class Degree
{
public:
	Degree(f32 degree = 0.0f);
	Degree(const Degree& degree);
	Degree(const Radian& radian);
	
	f32 Get() const;

	Degree& operator= (const Degree& degree);
	Degree& operator= (const Radian& radian);

	Degree& operator+= (const Degree& degree);
	Degree& operator+= (const Radian& radian);

	const Degree operator+ (const Degree& degree);
	const Degree operator+ (const Radian& radian);

	Degree& operator-= (const Degree& degree);
	Degree& operator-= (const Radian& radian);

	const Degree operator- (const Degree& degree);
	const Degree operator- (const Radian& radian);

	Degree& operator*= (const Degree& degree);
	Degree& operator*= (const Radian& radian);

	const Degree operator* (const Degree& degree);
	const Degree operator* (const Radian& radian);

	Degree& operator/= (const Degree& degree);
	Degree& operator/= (const Radian& radian);

	const Degree operator/ (const Degree& degree);
	const Degree operator/ (const Radian& radian);

private:
	f32 m_Degree;
};