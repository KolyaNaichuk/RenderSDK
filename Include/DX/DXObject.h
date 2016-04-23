#pragma once

#include "Common/Common.h"

template <typename T>
class DXObject
{
public:
	DXObject(T* pDXObject = nullptr)
		: m_pDXObject(pDXObject)
	{}
	DXObject(const DXObject& other) = delete;
	DXObject& operator= (const DXObject& other) = delete;

protected:
	~DXObject()
	{
		SafeRelease(m_pDXObject);
	}

public:
	T* GetDXObject()
	{
		return m_pDXObject;
	}
	T** GetDXObjectAddress()
	{
		return &m_pDXObject;
	}
	void SetName(LPCWSTR pName)
	{
		DXVerify(m_pDXObject->SetName(pName));
	}

private:
	T* m_pDXObject;
};
