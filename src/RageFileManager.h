#ifndef RAGE_FILE_MANAGER_H
#define RAGE_FILE_MANAGER_H

#include <unordered_set>
#include <vector>


/** @brief Constants for working with the RageFileManager. */
namespace RageFileManagerUtil
{
	extern std::string sDirOfExecutable;
}

class RageFileDriver;
class RageFileBasic;
struct lua_State;

bool ilt( const std::string &a, const std::string &b );
bool ieq( const std::string &a, const std::string &b );

/** @brief File utilities and high-level manager for RageFile objects. */
class RageFileManager
{
public:
	RageFileManager( const std::string &argv0 );
	~RageFileManager();
	void MountInitialFilesystems();
	void MountUserFilesystems();

	void GetDirListing( const std::string &sPath, std::vector<std::string> &AddTo, bool bOnlyDirs, bool bReturnPathToo );
	void GetDirListingWithMultipleExtensions(const std::string &sPath,
		std::vector<std::string> const& ExtensionList, std::vector<std::string> &AddTo,
		bool bOnlyDirs= false, bool bReturnPathToo= false);
	bool Move( const std::string &fromPath, const std::string &toPath );
	bool Copy( const std::string &fromPath, const std::string &toPath );
	bool Remove( const std::string &sPath );
	bool DeleteRecursive( const std::string &sPath );
	void CreateDir( const std::string &sDir );

	enum FileType { TYPE_FILE, TYPE_DIR, TYPE_NONE };
	FileType GetFileType( const std::string &sPath );

	bool IsAFile( const std::string &sPath );
	bool IsADirectory( const std::string &sPath );
	bool DoesFileExist( const std::string &sPath );

	int GetFileSizeInBytes( const std::string &sPath );
	int GetFileHash( const std::string &sPath );

	/**
	 * @brief Get the absolte path from the VPS.
	 * @param path the VPS path.
	 * @return the absolute path. */
	std::string ResolvePath(const std::string &path);

	bool Mount( const std::string &sType, const std::string &sRealPath, const std::string &sMountPoint );
	void Unmount( const std::string &sType, const std::string &sRoot, const std::string &sMountPoint );

	/* Change the root of a filesystem.  Only a couple drivers support this; it's
	 * used to change memory card mountpoints without having to actually unmount
	 * the driver. */
	void Remount( std::string sMountpoint, std::string sPath );
	bool IsMounted( std::string MountPoint );
	struct DriverLocation
	{
		std::string Type, Root, MountPoint;
	};
	void GetLoadedDrivers( std::vector<DriverLocation> &asMounts );

	void FlushDirCache( const std::string &sPath = std::string() );

	/* Used only by RageFile: */
	RageFileBasic *Open( const std::string &sPath, int iMode, int &iError );
	void CacheFile( const RageFileBasic *fb, const std::string &sPath );

	/* Retrieve or release a reference to the low-level driver for a mountpoint. */
	RageFileDriver *GetFileDriver( std::string sMountpoint );
	void ReleaseFileDriver( RageFileDriver *pDriver );

	bool Unzip(const std::string &zipPath, std::string targetPath, int strip);

	// path protection
	void ProtectPath(const std::string& path);
	bool IsPathProtected(const std::string& path);

	// Lua
	void PushSelf( lua_State *L );

private:
	RageFileBasic *OpenForReading( const std::string &sPath, int iMode, int &iError );
	RageFileBasic *OpenForWriting( const std::string &sPath, int iMode, int &iError );

	std::unordered_set<std::string> m_protectedPaths;
};

extern RageFileManager *FILEMAN;

#endif

/*
 * Copyright (c) 2001-2004 Glenn Maynard, Chris Danford
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
