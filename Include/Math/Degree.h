#pragma once

#include "Math/Math.h"

class Radian;

class Degree
{
public:
	explicit Degree(f32 degree = 0.0f);
	Degree(const Degree& degree);
	Degree(const Radian& radian);
	
	f32 Get() const;

	Degree& operator= (f32 degree);
	Degree& operator= (const Degree& degree);
	Degree& operator= (const Radian& radian);

	Degree& operator+= (f32 degree);
	Degree& operator+= (const Degree& degree);
	Degree& operator+= (const Radian& radian);

	const Degree operator+ (const Degree& degree);
	const Degree operator+ (const Radian& radian);

	Degree& operator-= (f32 degree);
	Degree& operator-= (const Degree& degree);
	Degree& operator-= (const Radian& radian);

	const Degree operator- (const Degree& degree);
	const Degree operator- (const Radian& radian);

	Degree& operator*= (f32 degree);
	Degree& operator*= (const Degree& degree);
	Degree& operator*= (const Radian& radian);

	const Degree operator* (const Degree& degree);
	const Degree operator* (const Radian& radian);

	Degree& operator/= (f32 degree);
	Degree& operator/= (const Degree& degree);
	Degree& operator/= (const Radian& radian);

	const Degree operator/ (const Degree& degree);
	const Degree operator/ (const Radian& radian);

	const Degree operator- () const;

private:
	f32 m_Degree;
};

const Degree operator+ (f32 left, const Degree& right);
const Degree operator+ (const Degree& left, f32 right);
const Degree operator- (f32 left, const Degree& right);
const Degree operator- (const Degree& left, f32 right);
const Degree operator* (f32 left, const Degree& right);
const Degree operator* (const Degree& left, f32 right);
const Degree operator/ (f32 left, const Degree& right);
const Degree operator/ (const Degree& left, f32 right);