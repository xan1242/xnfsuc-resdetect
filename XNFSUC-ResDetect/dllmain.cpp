#include "stdio.h"
#include <windows.h>
#include "includes\injector\injector.hpp"
#include "includes\IniReader.h"

unsigned int MonitorToUse = 0;
unsigned int ActualResCount = 0;
unsigned int* ResolutionPointers;

struct ScreenRes
{
	unsigned int ResX;
	unsigned int ResY;
	unsigned int LangHash;
}MonitorRes[255];

char MonitorResString[255][64];

void InitConfig()
{
	CIniReader inireader("");
	MonitorToUse = inireader.ReadInteger("ResDetect", "MonitorIndex", 0);
}

void DetectResolutions(unsigned int MonitorIndex)
{
	unsigned int PreviousResX = 0;
	unsigned int PreviousResY = 0;
	DEVMODE dm = { 0 };
	DISPLAY_DEVICE dispdev = { 0 };

	dispdev.cb = 424;
	EnumDisplayDevices(NULL, MonitorIndex, &dispdev, 1);
	dm.dmSize = sizeof(dm);
	for (int iModeNum = 0; EnumDisplaySettings(dispdev.DeviceName, iModeNum, &dm) != 0; iModeNum++) {
		if ((dm.dmPelsWidth != PreviousResX) || (dm.dmPelsHeight != PreviousResY))
		{
			//printf("Mode #%d = %dx%d\n", ActualResCount, dm.dmPelsWidth, dm.dmPelsHeight);
			sprintf(MonitorResString[ActualResCount], "%dx%d", dm.dmPelsWidth, dm.dmPelsHeight);
			MonitorRes[ActualResCount].ResX = dm.dmPelsWidth;
			MonitorRes[ActualResCount].ResY = dm.dmPelsHeight;
			ActualResCount++;
		}
		PreviousResX = dm.dmPelsWidth;
		PreviousResY = dm.dmPelsHeight;
	}
}

char* GetPackedStringHook(unsigned int StringHash, bool Something)
{
	unsigned int CurrentResIndex;
	_asm mov CurrentResIndex, edi

	return MonitorResString[CurrentResIndex];
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		InitConfig();
		DetectResolutions(MonitorToUse);

		// allocate memory for resolution pointers
		ResolutionPointers = (unsigned int*)calloc(ActualResCount, sizeof(int));

		// patch res pointers
		injector::WriteMemory<unsigned int>(0x00555F9E, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x00555FC4, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x0059C04C, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x0059C068, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x0059C085, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x0059D103, (unsigned int)&ResolutionPointers, true);
		injector::WriteMemory<unsigned int>(0x0059D153, (unsigned int)&ResolutionPointers, true);

		injector::WriteMemory<unsigned int>(0x00555F7C, (unsigned int)&MonitorRes, true);
		injector::WriteMemory<unsigned int>(0x00768C62, (unsigned int)&MonitorRes, true);
		injector::WriteMemory<unsigned int>(0x00768C72, (unsigned int)&MonitorRes + 4, true);
		injector::WriteMemory<unsigned int>(0x00555FAF, (unsigned int)&MonitorRes[ActualResCount], true);
		injector::MakeCALL(0x0059D10B, GetPackedStringHook, true);
		injector::MakeNOP(0x00555F99, 2, true); // get rid of resolution check
		injector::MakeJMP(0x00555F85, 0x00555F94, true);
	}
	return TRUE;
}
