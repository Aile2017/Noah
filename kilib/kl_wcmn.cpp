//--- K.I.LIB ---
// kl_wcmn.h : windows-common-interface operatin

#include "stdafx.h"
#include "kilib.h"

void kiSUtil::switchCurDirToExeDir()
{
	char exepath[MAX_PATH+50];
	GetModuleFileName( NULL, exepath, MAX_PATH );
	char* lastslash = 0;
	for( char* p=exepath; *p; p=CharNext(p) )
		if( *p=='\\' || *p=='/' )
			lastslash = p;
	if(lastslash)
		*lastslash = '\0';
	SetCurrentDirectory(exepath);
}

static int CALLBACK __ki__ofp( HWND w, UINT m, LPARAM l, LPARAM d )
{
	if( m==BFFM_INITIALIZED && d )
		::SendMessage( w, BFFM_SETSELECTION, TRUE, d );
	return 0;
}

bool kiSUtil::getFolderDlg( char* buf, HWND par, const char* title, const char* def )
{
	// Set info
	BROWSEINFO bi;
	ki_memzero( &bi, sizeof(bi) );
	bi.hwndOwner      = par;
	bi.pszDisplayName = buf;
	bi.lpszTitle      = title;
	bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN;
	bi.lpfn           = __ki__ofp;
	bi.lParam         = reinterpret_cast<LPARAM>(def);

	// Show dialog
	LPITEMIDLIST id = ::SHBrowseForFolder( &bi );
	if( id==NULL )
		return false;
	::SHGetPathFromIDList( id, buf );
	app()->shellFree( id );

	// End
	return true;
}

void kiSUtil::getFolderDlgOfEditBox( HWND wnd, HWND par, const char* title )
{
	char str[MAX_PATH];
	::SendMessage( wnd, WM_GETTEXT, MAX_PATH, (LPARAM)str );
	char* l = str;
	for( char* x=str; *x; x=kiStr::next(x) )
		l=x;
	if( *l=='\\' || *l=='/' )
		*l='\0';
	if( getFolderDlg( str, par, title, str ) )
		::SendMessage( wnd, WM_SETTEXT, 0, (LPARAM)str );
}

void kiSUtil::msgLastError( const char* msg )
{
	char* pMsg;
	::FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,::GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&pMsg,0,NULL );
	if( msg )
		app()->msgBox( kiStr(msg) + "\r\n\r\n" + pMsg );
	else
		app()->msgBox( pMsg );
	::LocalFree( pMsg );
}

bool kiSUtil::exist( const char* fname )
{
	return 0xffffffff != ::GetFileAttributes( fname );
}

bool kiSUtil::isdir( const char* fname )
{
	DWORD attr = ::GetFileAttributes( fname );
	return attr!=0xffffffff && (attr&FILE_ATTRIBUTE_DIRECTORY);
}
