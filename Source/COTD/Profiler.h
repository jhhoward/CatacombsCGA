#pragma once

#include "Platform.h"
#include <memory.h>

#define WITH_PROFILER 1

#if WITH_PROFILER
#define PROFILE_SECTION(SectionType) \
	ProfilerSection profileSection__##SectionType(Profile_##SectionType);
#else
#define PROFILE_SECTION(SectionType)
#endif

enum ProfilerSectionType
{
	Profile_Draw,
	Profile_Tick,
	Profile_PlotWall,
	Profile_BlitWall,
	Profile_BlitSprite,
	Profile_CalcPortal,
	NumProfilerSectionTypes
};

class Profiler
{
public:
	static void ResetTimers()
	{
		memset(timerValue, 0, sizeof(unsigned long) * NumProfilerSectionTypes);
	}
	static unsigned long timerValue[NumProfilerSectionTypes];
};

class ProfilerSection
{
public:
	ProfilerSection(ProfilerSectionType inSectionType) : sectionType(inSectionType)
	{
		startTime = Platform::GetTime();
	}
	~ProfilerSection()
	{
		Profiler::timerValue[sectionType] += Platform::GetTime() - startTime;
	}

private:
	ProfilerSectionType sectionType;
	unsigned long startTime;
};

