//--- K.I.LIB ---
// kl_wcmn.h : windows-common-interface operatin

#ifndef AFX_KIWINCOMMON_H__0686721C_CAFB_4C2C_9FE5_0F482EA6A60B__INCLUDED_
#define AFX_KIWINCOMMON_H__0686721C_CAFB_4C2C_9FE5_0F482EA6A60B__INCLUDED_

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
// Shell utility class

class kiSUtil
{
public:
	// Save/restore current directory
	static void switchCurDirToExeDir();

	// "Select Folder" dialog
	static bool getFolderDlg( char* buf, HWND par, const char* title, const char* def );
	static void getFolderDlgOfEditBox( HWND wnd, HWND par, const char* title );

	// Return the system image list index for the icon of the given extension.
	static int getSysIcon( const char* ext );

	// Display last error
	static void msgLastError( const char* msg = NULL );

	// Create a shortcut to self
	static void createShortCut( const kiPath& at, const char* name );

	// Does the file exist?
	static bool exist( const char* fname );
	static bool isdir( const char* fname );

	// Move current directory to a safe location before LoadLibrary
	static HMODULE loadLibrary(LPCTSTR lpFileName)
	{
		char original_cur[MAX_PATH], sys[MAX_PATH];
		::GetCurrentDirectory(MAX_PATH, original_cur);
		::GetSystemDirectory(sys, MAX_PATH);
		::SetCurrentDirectory(sys);
		HMODULE han = ::LoadLibrary(lpFileName);
		::SetCurrentDirectory(original_cur);
		return han;
	}
};

#endif
