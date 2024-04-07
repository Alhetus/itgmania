#define CO_EXIST_WITH_MFC
#include "global.h"
#include "stdafx.h"
#include "SMPackageUtil.h"
#include "archutils/Win32/RegistryAccess.h"
#include "ProductInfo.h"
#include "RageUtil.h"
#include "RageFileManager.h"
#include "resource.h"
#include "LocalizedString.h"
#include "arch/Dialog/Dialog.h"
#include "StringUtil.h"

#include <cstddef>
#include <vector>

static const std::string SMPACKAGE_KEY = "HKEY_LOCAL_MACHINE\\Software\\" PRODUCT_ID "\\smpackage";
static const std::string INSTALLATIONS_KEY = "HKEY_LOCAL_MACHINE\\Software\\" PRODUCT_ID "\\smpackage\\Installations";

void SMPackageUtil::WriteGameInstallDirs( const std::vector<std::string>& asInstallDirsToWrite )
{
	RegistryAccess::CreateKey( INSTALLATIONS_KEY );

	for( unsigned i=0; i<100; i++ )
	{
		std::string sName = ssprintf("%d",i);
		std::string sValue;
		if( i < asInstallDirsToWrite.size() )
			sValue = asInstallDirsToWrite[i];

		RegistryAccess::SetRegValue( INSTALLATIONS_KEY, sName, sValue );
	}
}

void SMPackageUtil::GetGameInstallDirs( std::vector<std::string>& asInstallDirsOut )
{
	asInstallDirsOut.clear();

	for( int i=0; i<100; i++ )
	{
		std::string sName = ssprintf("%d",i);

		std::string sPath;
		if( !RegistryAccess::GetRegValue(INSTALLATIONS_KEY, sName, sPath) )
			continue;

		if( sPath == "" )	// blank entry
			continue;	// skip

		if( !IsValidInstallDir(sPath) )
			continue;	// skip

		asInstallDirsOut.push_back( sPath );
	}

	// while we're at it, write to clean up stale entries
	WriteGameInstallDirs( asInstallDirsOut );
}

void SMPackageUtil::AddGameInstallDir( const std::string &sNewInstallDir )
{
	std::vector<std::string> asInstallDirs;
	GetGameInstallDirs( asInstallDirs );

	bool bAlreadyInList = false;
	for( unsigned i=0; i<asInstallDirs.size(); i++ )
	{
		if( StringUtil::EqualsNoCase(asInstallDirs[i], sNewInstallDir) )
		{
			bAlreadyInList = true;
			break;
		}
	}

	if( !bAlreadyInList )
		asInstallDirs.push_back( sNewInstallDir );

	WriteGameInstallDirs( asInstallDirs );
}

void SMPackageUtil::SetDefaultInstallDir( int iInstallDirIndex )
{
	// move the specified index to the top of the list
	std::vector<std::string> asInstallDirs;
	GetGameInstallDirs( asInstallDirs );
	ASSERT( iInstallDirIndex >= 0  &&  iInstallDirIndex < (int)asInstallDirs.size() );
	std::string sDefaultInstallDir = asInstallDirs[iInstallDirIndex];
	asInstallDirs.erase( asInstallDirs.begin()+iInstallDirIndex );
	asInstallDirs.insert( asInstallDirs.begin(), sDefaultInstallDir );
	WriteGameInstallDirs( asInstallDirs );
}

void SMPackageUtil::SetDefaultInstallDir( const std::string &sInstallDir )
{
	std::vector<std::string> asInstallDirs;
	GetGameInstallDirs( asInstallDirs );

	for( unsigned i=0; i<asInstallDirs.size(); i++ )
	{
		if( StringUtil::EqualsNoCase(asInstallDirs[i], sInstallDir) )
		{
			SetDefaultInstallDir( i );
			break;
		}
	}
}

bool SMPackageUtil::IsValidInstallDir( const std::string &sInstallDir )
{
	return DoesOsAbsoluteFileExist( sInstallDir + "/Songs" );
}

bool SMPackageUtil::GetPref( const std::string &name, bool &val )
{
	return RegistryAccess::GetRegValue( SMPACKAGE_KEY, name, val );
}

bool SMPackageUtil::SetPref( const std::string &name, bool val )
{
	return RegistryAccess::SetRegValue( SMPACKAGE_KEY, name, val );
}

/* Get a package directory.  For most paths, this is the first two components.  For
 * songs and note skins, this is the first three. */
std::string SMPackageUtil::GetPackageDirectory(const std::string &path)
{
	// ignore CVS/.svn files:
	if( path.find("CVS") != std::string::npos )
		return "";
	if( path.find(".svn") != std::string::npos )
		return "";

	std::vector<std::string> Parts;
	split( path, "\\", Parts );

	unsigned NumParts = 2;
	// Songs/group/single_song, NoteSkins/gametype/single_noteskin:
	if( !StringUtil::CompareNoCase(Parts[0], "Songs") || !StringUtil::CompareNoCase(Parts[0], "NoteSkins") )
		NumParts = 3;
	if( Parts.size() < NumParts )
		return "";

	Parts.erase(Parts.begin() + NumParts, Parts.end());

	std::string ret = join( "\\", Parts );
	if( !IsADirectory(ret) )
		return "";
	return ret;
}

bool SMPackageUtil::IsValidPackageDirectory( const std::string &path )
{
	/* Make sure the path contains only second-level directories, and doesn't
	 * contain any ".", "..", "...", etc. dirs. */
	std::vector<std::string> Parts;
	split( path, "\\", Parts, true );
	if( Parts.size() == 0 )
		return false;

	/* Make sure we're not going to "uninstall" an entire Songs subfolder. */
	unsigned NumParts = 2;
	if( !StringUtil::CompareNoCase(Parts[0], "songs") )
		NumParts = 3;
	if( Parts.size() < NumParts )
		return false;

	/* Make sure the path doesn't contain any ".", "..", "...", etc. dirs. */
	for( unsigned i = 0; i < Parts.size(); ++i )
		if( Parts[i][0] == '.' )
			return false;

	return true;
}

static LocalizedString COULD_NOT_FIND( "SMPackageUtil", "Could not find '%s'." );
bool SMPackageUtil::LaunchGame()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO	si;
	ZeroMemory( &si, sizeof(si) );

	std::string sFile = "Program\\" PRODUCT_FAMILY ".exe";

	BOOL bSuccess = CreateProcess(
		sFile.c_str(),	// pointer to name of executable module
		NULL,	// pointer to command line string
		NULL,  // process security attributes
		NULL,   // thread security attributes
		false,  // handle inheritance flag
		0, // creation flags
		NULL,  // pointer to new environment block
		NULL,   // pointer to current directory name
		&si,  // pointer to STARTUPINFO
		&pi  // pointer to PROCESS_INFORMATION
	);
	if( !bSuccess )
	{
		std::string sError = ssprintf( COULD_NOT_FIND.GetValue().c_str(), sFile.c_str() );
		Dialog::OK( sError );
		return false;
	}

	return true;
}

std::string SMPackageUtil::GetLanguageDisplayString( const std::string &sIsoCode )
{
	const LanguageInfo *li = GetLanguageInfo( sIsoCode );
	return ssprintf( "%s (%s)", li ? li->szIsoCode:sIsoCode.c_str(), li->szEnglishName );
}

std::string SMPackageUtil::GetLanguageCodeFromDisplayString( const std::string &sDisplayString )
{
	std::string s = sDisplayString;
	// strip the space and everything after
	std::size_t iSpace = s.find(' ');
	ASSERT( iSpace != s.npos );
	s.erase( s.begin()+iSpace, s.end() );
	return s;
}

void SMPackageUtil::StripIgnoredSmzipFiles( std::vector<std::string> &vsFilesInOut )
{
	for( int i=vsFilesInOut.size()-1; i>=0; i-- )
	{
		const std::string &sFile = vsFilesInOut[i];

		bool bEraseThis = false;
		bEraseThis |= EndsWith( sFile, "smzip.ctl" );
		bEraseThis |= EndsWith( sFile, ".old" );
		bEraseThis |= EndsWith( sFile, "Thumbs.db" );
		bEraseThis |= EndsWith( sFile, ".DS_Store" );
		bEraseThis |= (sFile.find("CVS") != std::string::npos);

		if( bEraseThis )
			vsFilesInOut.erase( vsFilesInOut.begin()+i );
	}
}

bool SMPackageUtil::DoesOsAbsoluteFileExist( const std::string &sOsAbsoluteFile )
{
#if defined(WIN32)
	DWORD dwAttr = ::GetFileAttributes( sOsAbsoluteFile.c_str() );
	return bool(dwAttr != (DWORD)-1);
#endif
}


static const std::string TEMP_MOUNT_POINT = "/@package/";

RageFileOsAbsolute::~RageFileOsAbsolute()
{
	if( !m_sOsDir.empty() )
		FILEMAN->Unmount( "dir", m_sOsDir, TEMP_MOUNT_POINT );
}

bool RageFileOsAbsolute::Open( const std::string& path, int mode )
{
	if( !m_sOsDir.empty() )
		FILEMAN->Unmount( "dir", m_sOsDir, TEMP_MOUNT_POINT );

	m_sOsDir = path;
	std::size_t iStart = m_sOsDir.find_last_of( "/\\" );
	ASSERT( iStart != m_sOsDir.npos );
	m_sOsDir.erase( m_sOsDir.begin()+iStart, m_sOsDir.end() );

	FILEMAN->Mount( "dir", m_sOsDir, TEMP_MOUNT_POINT );
	std::string sFileName = path.substr( m_sOsDir.size() );
	return RageFile::Open( TEMP_MOUNT_POINT+sFileName, mode );
}

/*
 * (c) 2002-2005 Chris Danford
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
