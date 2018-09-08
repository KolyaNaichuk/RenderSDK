#pragma once

#include "Common/Common.h"

class CPUProfiler
{
public:
	CPUProfiler(u32 maxNumProfiles);

	void StartFrame();
	void EndFrame();

	u32 StartProfile(const char* pProfileName);
	void EndProfile(u32 profileIndex);

	void OutputToConsole();

private:
	struct ProfileData
	{
		std::string m_Name;
		enum { kNumTimeSamples = 64 };
		f64 m_TimeSamples[kNumTimeSamples] = {};
		u32 m_CurrentSampleIndex = 0;
		f64 m_MaxTime = 0.0;
		f64 m_AvgTime = 0.0;
	};
	struct TimeInterval
	{
		LARGE_INTEGER m_StartCounter;
		LARGE_INTEGER m_EndCounter;
	};
	
	std::vector<ProfileData> m_Profiles;
	std::vector<TimeInterval> m_TimeIntervals;
	u32 m_NumUsedProfiles = 0;
	u32 m_MaxNumQueries = 0;
	LARGE_INTEGER m_Frequency;
};