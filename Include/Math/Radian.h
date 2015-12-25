#pragma once

#include "Math/Math.h"

class Degree;

class Radian
{
public:
	Radian(f32 radian = 0.0f);
	Radian(const Radian& radian);
	Radian(const Degree& degree);

	f32 Get() const;

	Radian& operator= (const Radian& radian);
	Radian& operator= (const Degree& degree);

	Radian& operator+= (const Radian& radian);
	Radian& operator+= (const Degree& degree);

	const Radian operator+ (const Radian& radian);
	const Radian operator+ (const Degree& degree);

	Radian& operator-= (const Radian& radian);
	Radian& operator-= (const Degree& degree);

	const Radian operator- (const Radian& radian);
	const Radian operator- (const Degree& degree);

	Radian& operator*= (const Radian& radian);
	Radian& operator*= (const Degree& degree);

	const Radian operator* (const Radian& radian);
	const Radian operator* (const Degree& degree);

	Radian& operator/= (const Radian& radian);
	Radian& operator/= (const Degree& degree);

	const Radian operator/ (const Radian& radian);
	const Radian operator/ (const Degree& degree);

private:
	f32 m_Radian;
};