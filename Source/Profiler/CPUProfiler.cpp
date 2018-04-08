#include "Profiler/CPUProfiler.h"
#include "Math/Math.h"

CPUProfiler::CPUProfiler(u32 maxNumProfiles)
{
	VerifyWinAPIResult(QueryPerformanceFrequency(&m_Frequency));
	
	m_Profiles.resize(maxNumProfiles);
	m_TimeIntervals.resize(maxNumProfiles);
}

void CPUProfiler::StartFrame()
{
}

void CPUProfiler::EndFrame()
{
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		const TimeInterval& timeInterval = m_TimeIntervals[profileIndex];
		f64 timeInMilliseconds = (f64(timeInterval.m_EndCounter.QuadPart - timeInterval.m_StartCounter.QuadPart) / f64(m_Frequency.QuadPart)) * 1000.0;

		ProfileData& profileData = m_Profiles[profileIndex];		
		profileData.m_TimeSamples[profileData.m_CurrentSampleIndex] = timeInMilliseconds;
		profileData.m_CurrentSampleIndex = (profileData.m_CurrentSampleIndex + 1) % ProfileData::kNumTimeSamples;

		f64 maxTime = 0.0;
		f64 avgTime = 0.0;
		u32 numAvgSamples = 0;

		for (u32 sampleIndex = 0; sampleIndex < ProfileData::kNumTimeSamples; ++sampleIndex)
		{
			if (profileData.m_TimeSamples[sampleIndex] > 0.0f)
			{
				maxTime = Max(maxTime, profileData.m_TimeSamples[sampleIndex]);

				avgTime += profileData.m_TimeSamples[sampleIndex];
				++numAvgSamples;
			}
		}

		if (numAvgSamples > 0)
			avgTime /= f64(numAvgSamples);

		profileData.m_MaxTime = maxTime;
		profileData.m_AvgTime = avgTime;
	}
}

u32 CPUProfiler::StartProfile(const char* pProfileName)
{
	assert(pProfileName != nullptr);

	u32 profileIndex = 0;
	for (; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		if (m_Profiles[profileIndex].m_Name == pProfileName)
			break;
	}
	if (profileIndex == m_NumUsedProfiles)
	{
		assert(m_NumUsedProfiles < m_Profiles.size());

		profileIndex = m_NumUsedProfiles++;
		m_Profiles[profileIndex].m_Name = pProfileName;
	}

	VerifyWinAPIResult(QueryPerformanceCounter(&m_TimeIntervals[profileIndex].m_StartCounter));
	return profileIndex;
}

void CPUProfiler::EndProfile(u32 profileIndex)
{
	assert(profileIndex < m_NumUsedProfiles);
	VerifyWinAPIResult(QueryPerformanceCounter(&m_TimeIntervals[profileIndex].m_EndCounter));
}

void CPUProfiler::OutputToConsole()
{
	enum class SortOrder
	{
		RenderOrder,
		MaxToMinAvgTimeOrder
	};
	const SortOrder sortOrder = SortOrder::MaxToMinAvgTimeOrder;

	std::vector<const ProfileData*> profiles(m_NumUsedProfiles);
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
		profiles[profileIndex] = &m_Profiles[profileIndex];

	std::function<bool(const ProfileData*, const ProfileData*)> comp;
	if (sortOrder == SortOrder::RenderOrder)
	{
		comp = [](const ProfileData* left, const ProfileData* right)
		{
			return false;
		};
	}
	else if (sortOrder == SortOrder::MaxToMinAvgTimeOrder)
	{
		comp = [](const ProfileData* left, const ProfileData* right)
		{
			return (left->m_AvgTime > right->m_AvgTime);
		};
	}
	else
	{
		assert(false);
	}
	std::sort(profiles.begin(), profiles.end(), comp);

	const u8 outputBufferSize = 255;
	char outputBuffer[outputBufferSize];

	OutputDebugStringA("================================= CPU Profiler ==========================================\n");
	for (u32 profileIndex = 0; profileIndex < m_NumUsedProfiles; ++profileIndex)
	{
		const ProfileData* pProfileData = profiles[profileIndex];

		std::snprintf(outputBuffer, outputBufferSize, "%s: avg: %.3f ms, max: %.3f ms\n",
			pProfileData->m_Name.c_str(), pProfileData->m_AvgTime, pProfileData->m_MaxTime);

		OutputDebugStringA(outputBuffer);
	}
	OutputDebugStringA("=========================================================================================\n");
}
