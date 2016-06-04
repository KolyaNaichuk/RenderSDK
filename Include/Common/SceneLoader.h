#pragma once

class Scene;

enum CornellBoxSettings
{
	CornellBoxSettings_Original,
	CornellBoxSettings_Test1,
	CornellBoxSettings_Test2
};

class SceneLoader
{
public:
	static Scene* LoadCornellBox(CornellBoxSettings settings = CornellBoxSettings_Original);
};