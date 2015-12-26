#include "Common/SceneObject.h"

SceneObject::SceneObject(const std::string& name)
	: m_Name(name)
{
}

const std::string& SceneObject::GetName() const
{
	return m_Name;
}

void SceneObject::SetName(const std::string& name)
{
	m_Name = name;
}

Transform& SceneObject::GetTransform()
{
	return m_Transform;
}

const Transform& SceneObject::GetTransform() const
{
	return m_Transform;
}
