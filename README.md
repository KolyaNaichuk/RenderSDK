RenderSDK is a C++/Direct3D 12 project I am working in my spare time.
I started it in August, 2015, when Direct12 API became publicly available with release of Windows 10.

My objective with the project was to gain practical experience with Direct3D 12 API.
I wanted to have a set of ready and easy to use utilities to implement and experiment with graphics effects, taking benefit of the new API features.

<b>Building the project</b>  
The project is setup for Visual Studio 2015, Debug, x86 mode.  
You might also need to update Target Platform Version (Configuration Properties/General) for the projects in the solution based on your current Windows SDK. Version 10.0.15063.0 is used by default.

In the current development state, it provides the following packages:

<b>D3DWrapper</b>

Thin Direct3D 12 wrappers around the native API.

<b>Math</b>

Essential mathematical things to deal with
- vectors
- matrices
- quaternions
- transformations
- spherical harmonics
- intersection tests, etc.

<b>RenderPasses</b>

In particular, it is responsible for setting up pipeline state object associated with the render pass and recording the draw/compute commands.

<b>Samples</b>

<b>HelloRectangle</b>

Demonstrates core functionality of Direct3D 12 API in action, such as using
- command queue and command lists
- index and vertex buffers
- descriptor table and descriptor heap
- uploading data to the GPU and handling resource transitions
- command lists execution synchronization.

![Alt text](/Samples/HelloRectangle/Screenshots/Screenshot.png?raw=true)

<b>Dynamic Global Illumination</b>

I started this sample to play around with global illumnation using voxel representation of the scene to compute indirect lighting. Here comes a short overview of what it does.

1.The render frame starts with detecting visible meshes from camera using compute shader. Each spawned thread tests mesh AABB against camera frustum. As a result of the pass, we populate two buffers: the first one containing number of visible meshes and the second one - visible mesh IDs.

2.After, we detect point lights which affect visible meshes from camera using compute shader. Again, each spawned thread tests point light affecting volume (bounding sphere) against camera frustum. As a result of the pass, we generate two buffers: the first one containing number of visible point lights and the second one - visible light IDs.

3.We detect spot lights which affect visible meshes from camera using compute shader. Each spawned thread tests spot light affecting volume against camera frustum. As a spot light volume, bounding sphere around spot light cone is used.
As a result of the pass, we produce two buffers: the first one containing number of visible spot lights and the second one - visible light IDs.

4.Based on detected visible meshes we generate indirect draw commands to render g-buffer using compute shader. The indirect draw argument struct looks the following way:
``````
struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};


struct DrawMeshCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};
``````

root32BitConstant is used to pass material ID associated with a particular mesh. The rest of the arguments should be self-explanatory.

5.Based on generated indirect draw commands, we render G-buffer using execute indirect.

6.After rendering G-buffer, we proceed to tiled light culling using compute shader. Specifically, we split our screen into tiles 16x16 and test each tile overlap against bounding volumes of visible lights. As a result, we generate two buffers: the first buffer containing overlapping light IDs and the second one - light IDs range associated with each tile from the previous buffer.

7.As soon as we have completed rendering G-Buffer, we start preparing indirect draw commands to render shadow maps for light sources using compute shader. In particular, we spawn as many thread groups as we have detected visible meshes. Each thread group tests mesh AABB against all visible point and spot light bounds, outputting overlapping point and spot light IDs for that shadow caster to the buffer. When the list of overlapping lights has been populated, one of the threads from the thread group will create indirect draw command to draw the shadow caster.

The indirect draw argument struct looks the following way:
``````
struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawMeshCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};
``````
root32BitConstant is used to transfer light IDs offset for the shadow caster in the resulting buffer.  
instanceCount specifies how many instances of the shadow caster you would like to draw. In our case, it will be equal to number of lights affecting that shadow caster.

8.To render shadow maps, Tiled Shadow Map approach from [5] is utilized. Tiled Shadow Map technique allows to render shadow maps for light sources in one indirect draw call. As the name suggests, the shadow map is split into tiles, where each tile defines shadow map region for one light. To render geometry to a particular shadow map tile, additional clip space translation to the vertex position is applied, after it has been transformed into clip space. Apart from this, we need to ensure that while rendering geometry to a concrete tile, we do not overwrite data of the neighboring tiles by the geometry expanding beyond the light view frustum. As proposed in [5], I exploit programmable clipping by specifying custom clipping plane distance (SV_ClipDistance) to clip the geometry outside light view frustum.

Step 6 mainly involves depth test and writing into the depth texture; whereas step 7 is primarily heavy on arithmetic instructions usage. These two steps are very good candidates to be performed in parallel using async compute queue. In my current implementation, they are executed on the same graphics queue but the intention is to move step 7 to compute queue and execute in parallel.

9.After, I apply tiled shading based on found lights per each screen tile, using Phong shading mode.

10.To calculate indirect lighting, voxel grid of the scene is generated as described in [1] and [4]. Specifically, we rasterize scene geometry with disabled color writing and depth test. In the geometry shader we select view matrix (either along X, Y or Z axis) for which triangle has the biggest area to get the highest number of rasterized pixels for the primitive. The rasterized pixels are output to the buffer, representing grid cells, from pixel shader.
To avoid race conditions between multiple threads writing to the same grid cell, I take advantage of rasterized ordered view, which provides atomic and ordered access to the resource.
Not to lose details of pixels which are only partially covered by the triangle, I enable conservative rasterization, which guarantees that pixel shader will be invoked for the pixel even if the primitive does not cover pixel center location.
After voxel grid construction, the voxels are illuminated by each light and converted into virtual point lights. Finally, the virtual point lights are propagated within the grid to generate indirect illumination,
which is later combined with already computed direct illumination.

![Alt text](/Samples/DynamicGI/Screenshots/DirectOnly.png?raw=true)
![Alt text](/Samples/DynamicGI/Screenshots/DirectAndIndirect.png?raw=true)

<b>Used Resources:</b>

[1] OpenGL Insights. Cyril Crassin and Simon Green, Octree-Based Sparse Voxelization Using the GPU Hardware Rasterization  
[2] Section on comulative moving average https://en.wikipedia.org/wiki/Moving_average  
[3] The Basics of GPU Voxelization https://developer.nvidia.com/content/basics-gpu-voxelization  
[4] GPU Pro 4. Hawar Doghramachi, Rasterized Voxel-Based Dynamic Global Illumination  
[5] GPU Pro 6. Hawar Doghramachi, Tile-Based Omnidirectional Shadows
[6] GPU Pro 2. Anton Kaplanyan, Wolfgang Engel and Carsten Dachsbacher, Diffuse Global Illumination with Temporally Coherent Light Propagation Volumes
