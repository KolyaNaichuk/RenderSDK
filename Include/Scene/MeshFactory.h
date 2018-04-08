#pragma once

class Mesh;

enum VertexDataFlags
{
	VertexDataFlag_Position = 1,
	VertexDataFlag_Normal = 2,
	VertexDataFlag_TexCoord = 4
};

class MeshFactory
{
public:
	static Mesh* CreateSphere(VertexDataFlags dataFlags);
	static Mesh* CreateTorus(VertexDataFlags dataFlags);
	static Mesh* CreateCube(VertexDataFlags dataFlags);
	static Mesh* CreateCylinder(VertexDataFlags dataFlags);
};