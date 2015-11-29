#pragma once

class MeshData;

enum VertexDataFlags
{
	VertexDataFlag_Position = 1,
	VertexDataFlag_Normal = 2,
	VertexDataFlag_TexCoord = 4
};

class MeshDataFactory
{
public:
	static MeshData* CreateSphereData(VertexDataFlags dataFlags);
	static MeshData* CreateTorusData(VertexDataFlags dataFlags);
	static MeshData* CreateCubeData(VertexDataFlags dataFlags);
	static MeshData* CreateCylinderData(VertexDataFlags dataFlags);
};