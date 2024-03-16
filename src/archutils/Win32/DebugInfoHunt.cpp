#include "global.h"
#include "DebugInfoHunt.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "VideoDriverInfo.h"
#include "RegistryAccess.h"

#include <vector>

#include <windows.h>
#include <mmsystem.h>


static void LogVideoDriverInfo( VideoDriverInfo info )
{
	LOG->Info( "Video driver: %s [%s]", info.sDescription.c_str(), info.sProvider.c_str() );
	LOG->Info( "              %s, %s [%s]", info.sVersion.c_str(), info.sDate.c_str(), info.sDeviceID.c_str() );
}

static void GetMemoryDebugInfo()
{
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);

	LOG->Info("Memory: %imb total, %imb swap (%imb swap avail)",
		mem.dwTotalPhys / 1048576,
		mem.dwTotalPageFile / 1048576,
		mem.dwAvailPageFile / 1048576);
}

static void GetDisplayDriverDebugInfo()
{
	std::string sPrimaryDeviceName = GetPrimaryVideoName();

	if( sPrimaryDeviceName == "" )
		LOG->Info( "Primary display driver could not be determined." );

	bool LoggedSomething = false;
	for( int i=0; true; i++ )
	{
		VideoDriverInfo info;
		if( !GetVideoDriverInfo(i, info) )
			break;

		if( sPrimaryDeviceName == "" )	// failed to get primary display name (NT4)
		{
			LogVideoDriverInfo( info );
			LoggedSomething = true;
		}
		else if( info.sDescription == sPrimaryDeviceName )
		{
			LogVideoDriverInfo( info );
			LoggedSomething = true;
			break;
		}
	}
	if( !LoggedSomething )
	{
		LOG->Info( "Primary display driver: %s", sPrimaryDeviceName.c_str() );
		LOG->Warn("Couldn't find primary display driver; logging all drivers");

		for( int i=0; true; i++ )
		{
			VideoDriverInfo info;
			if( !GetVideoDriverInfo(i, info) )
				break;

			LogVideoDriverInfo( info );
		}
	}
}

static std::string wo_ssprintf( MMRESULT err, const char *fmt, ...)
{
	char buf[MAXERRORLENGTH];
	waveOutGetErrorText(err, buf, MAXERRORLENGTH);

	va_list	va;
	va_start(va, fmt);
	std::string s = vssprintf( fmt, va );
	va_end(va);

	return s += ssprintf( "(%s)", buf );
}

static void GetDriveDebugInfo()
{
	/*
	 * HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\Scsi\
	 *    Scsi Port *\
	 *      DMAEnabled  0 or 1
	 *      Driver      "Ultra", "atapi", etc
	 *      Scsi Bus *\
	 *	     Target Id *\
	 *	 	   Logical Unit Id *\
	 *		     Identifier  "WDC WD1200JB-75CRA0"
	 *			 Type        "DiskPeripheral"
	 */
	std::vector<std::string> Ports;
	if( !RegistryAccess::GetRegSubKeys( "HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\Scsi", Ports ) )
		return;

	for( unsigned i = 0; i < Ports.size(); ++i )
	{
		int DMAEnabled = -1;
		RegistryAccess::GetRegValue( Ports[i], "DMAEnabled", DMAEnabled );

		std::string Driver;
		RegistryAccess::GetRegValue( Ports[i], "Driver", Driver );

		std::vector<std::string> Busses;
		if( !RegistryAccess::GetRegSubKeys( Ports[i], Busses, "Scsi Bus .*" ) )
			continue;

		for( unsigned bus = 0; bus < Busses.size(); ++bus )
		{
			std::vector<std::string> TargetIDs;
			if( !RegistryAccess::GetRegSubKeys( Busses[bus], TargetIDs, "Target Id .*" ) )
				continue;

			for( unsigned tid = 0; tid < TargetIDs.size(); ++tid )
			{
				std::vector<std::string> LUIDs;
				if( !RegistryAccess::GetRegSubKeys( TargetIDs[tid], LUIDs, "Logical Unit Id .*" ) )
					continue;

				for( unsigned luid = 0; luid < LUIDs.size(); ++luid )
				{
					std::string Identifier;
					RegistryAccess::GetRegValue( LUIDs[luid], "Identifier", Identifier );
					TrimRight( Identifier );
					LOG->Info( "Drive: \"%s\" Driver: %s DMA: %s",
						Identifier.c_str(), Driver.c_str(), DMAEnabled == 1? "yes":DMAEnabled == -1? "N/A":"NO" );
				}
			}
		}
	}
}

static void GetWindowsVersionDebugInfo()
{
	// Detect operating system.
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning( push )
#pragma warning( disable : 4996 )
	if (!GetVersionEx(&ovi))
#pragma warning( pop )
	{
		LOG->Info("GetVersionEx failed!");
		return;
	}

	std::string Ver = ssprintf("Windows %i.%i (", ovi.dwMajorVersion, ovi.dwMinorVersion);
	if(ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if(ovi.dwMinorVersion == 0)
			Ver += "Win95";
		else if(ovi.dwMinorVersion == 10)
			Ver += "Win98";
		else if(ovi.dwMinorVersion == 90)
			Ver += "WinME";
		else
			Ver += "unknown 9x-based";
	}
	else if(ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if(ovi.dwMajorVersion == 4 && ovi.dwMinorVersion == 0)
			Ver += "WinNT 4.0";
		else if(ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 0)
			Ver += "Win2000";
		else if(ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 1)
			Ver += "WinXP";
		else if(ovi.dwMajorVersion == 5 && ovi.dwMinorVersion == 2)
		{
			Ver += "WinServer2003";
			// todo: check for R2
			/*
			if(GetSystemMetrics(SM_SERVERR2) != 0)
				Ver += "R2";
			*/
		}
		else if(ovi.dwMajorVersion == 6 && ovi.dwMinorVersion == 0)
		{
			Ver += "Vista";
			// todo: make this check work
			/*
			if(ovi.wProductType == VER_NT_WORKSTATION)
				Ver += "Vista";
			else
				Ver += "WinServer2008";
			*/
		}
		else if(ovi.dwMajorVersion == 6 && ovi.dwMinorVersion == 1)
		{
			Ver += "Win7";
			// todo: make this check work
			/*
			if(ovi.wProductType == VER_NT_WORKSTATION)
				Ver += "Win7";
			else
				Ver += "WinServer2008 R2";
			*/
		}
		else if(ovi.dwMajorVersion == 6 && ovi.dwMinorVersion == 2)
		{
			Ver += "Win8";
			// todo: make this check work
			/*
			if(ovi.wProductType == VER_NT_WORKSTATION)
				Ver += "Win7";
			else
				Ver += "WinServer2008 R2";
			*/
		}
		else
			Ver += "unknown NT-based";
	} else Ver += "???";

	Ver += ssprintf(") build %i [%s]", ovi.dwBuildNumber & 0xffff, ovi.szCSDVersion);
	LOG->Info("%s", Ver.c_str());
}

static void GetSoundDriverDebugInfo()
{
	int cnt = waveOutGetNumDevs();

	for(int i = 0; i < cnt; ++i)
	{
		WAVEOUTCAPS caps;

		MMRESULT ret = waveOutGetDevCaps(i, &caps, sizeof(caps));
		if(ret != MMSYSERR_NOERROR)
		{
			LOG->Info(wo_ssprintf(ret, "waveOutGetDevCaps(%i) failed", i).c_str());
			continue;
		}
		LOG->Info("Sound device %i: %s, %i.%i, MID %i, PID %i %s", i, caps.szPname,
			HIBYTE(caps.vDriverVersion),
			LOBYTE(caps.vDriverVersion),
			caps.wMid, caps.wPid,
			caps.dwSupport & WAVECAPS_SAMPLEACCURATE? "":"(INACCURATE)");
	}
}

void SearchForDebugInfo()
{
	GetWindowsVersionDebugInfo();
	GetMemoryDebugInfo();
	GetDisplayDriverDebugInfo();
	GetDriveDebugInfo();
	GetSoundDriverDebugInfo();
}

/*
 * (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
