#pragma once

class Scene;

Scene* LoadSceneFromOBJFile(const wchar_t* pFilePath, bool use32BitIndices = false);
