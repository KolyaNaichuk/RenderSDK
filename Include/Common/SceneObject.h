#pragma once

#include "Common/Common.h"
#include "Math/Transform.h"

class SceneObject
{
public:
	SceneObject(const std::string& name);

	const std::string& GetName() const;
	void SetName(const std::string& name);

	Transform& GetTransform();
	const Transform& GetTransform() const;

private:
	std::string m_Name;
	Transform m_Transform;
};