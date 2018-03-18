#pragma once

class Scene;

class SceneLoader
{
public:
	static Scene* LoadCube();
	static Scene* LoadErato();
	static Scene* LoadCrytekSponza();
	static Scene* LoadDabrovicSponza();
	static Scene* LoadSibenik();
	static Scene* LoadCornellBox();
};