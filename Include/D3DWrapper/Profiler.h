#pragma once

#include "Common/Common.h"

class Buffer;
class CommandList;
class CommandQueue;
class QueryHeap;
struct RenderEnv;

struct ProfileData
{
	std::string m_Name;
	enum { kNumTimeSamples = 64 };
	f64 m_TimeSamples[kNumTimeSamples] = {};
	u32 m_CurrentSampleIndex = 0;
};

class Profiler
{
public:
	Profiler(RenderEnv* pRenderEnv, u32 maxNumProfiles, u32 renderLatency);
	~Profiler();

	void StartFrame(u32 currentFrameIndex);
	void EndFrame(CommandQueue* pCommandQueue);

	u32 StartProfile(CommandList* pCommandList, const char* pProfileName);
	void EndProfile(CommandList* pCommandList, u32 profileIndex);

	void OutputToConsole();

private:
	std::vector<ProfileData> m_Profiles;
	u32 m_NumUsedProfiles = 0;
	u32 m_MaxNumQueries = 0;
	u32 m_CurrentFrameIndex = 0;
	QueryHeap* m_pQueryHeap = nullptr;
	Buffer* m_pTimestampBuffer = nullptr;
};
