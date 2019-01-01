#pragma once

class Scene;
struct Matrix4f;

Scene* LoadSceneFromOBJFile(const wchar_t* pFilePath, const Matrix4f& worldMatrix, bool use32BitIndices = false);
