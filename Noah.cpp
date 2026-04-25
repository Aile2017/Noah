// Noah.cpp
// -- entrypoint etc for 'Noah'

#include "stdafx.h"
#include "NoahApp.h"
#include "resource.h"

// Process instance limit zone
class ProcessNumLimitZone
{
	HANDLE m_han;
public:
	ProcessNumLimitZone(int Max, const char* name)
		: m_han( ::CreateSemaphore( NULL, Max, Max, name ) )
	{
		if( m_han )
			::WaitForSingleObject( m_han, INFINITE );
		else
			kiSUtil::msgLastError("CreateSemaphore Failed");
	}
	~ProcessNumLimitZone()
	{
		if( m_han )
		{
			::ReleaseSemaphore( m_han, 1, NULL );
			::CloseHandle( m_han );
		}
	}
};

//----------------------------------------------//
//--------- Noah entry point ------------//
//----------------------------------------------//

void kilib_create_new_app()
{
	//-- Set application in kilib
	new CNoahApp;
}

void CNoahApp::run( kiCmdParser& cmd )
{
	//-- Initialize
	m_cnfMan.init();
	m_arcMan.init();

	//-- Retain command line parameters
	m_pCmd = &cmd;

	//-- Show settings dialog if no files given or Shift was held at startup
	if( cmd.param().len()==0 || keyPushed(VK_SHIFT) )
	{
		//-- Load-INI ( all )
		m_cnfMan.load( All );
		//-- Show settings screen
		m_cnfMan.dialog();
	}
	else
	{
		//-- Compression/extraction work
		do_cmdline( true );
	}

	//-- Exit processing
	m_tmpDir.remove();
}

//----------------------------------------------//
//------------- Compression/Extraction work --------------//
//----------------------------------------------//

bool CNoahApp::is_writable_dir( const kiPath& path )
{
	// Essentially, exclude CD-ROM/DVD-ROM drives.
	// Give up on excluding FDD and packet-write disks.

	// Treat RAMDISK, REMOTE, FIXED, UNKNOWN disks as writable
	UINT drv = path.getDriveType();
	if( drv==DRIVE_REMOVABLE || drv==DRIVE_CDROM )
	{
		// Dynamic-load since this API is not available on bare Win95
		typedef BOOL (WINAPI*pGDFSE)( LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER );
		pGDFSE pGetDiskFreeSpaceEx
			= (pGDFSE) ::GetProcAddress( ::GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA" );
		if( pGetDiskFreeSpaceEx )
		{
			// If free space is 0, treat as not writable
			ULARGE_INTEGER fs, dummy;
			pGetDiskFreeSpaceEx( path, &dummy, &dummy, &fs );
			if( fs.QuadPart == 0 )
				return false;
		}
	}
	return true;
}

void CNoahApp::do_cmdline( bool directcall )
{
	do_files( m_pCmd->param(), &m_pCmd->option(), !directcall );
}

void CNoahApp::do_files( const cCharArray& files,
						 const cCharArray* opts,
						 bool  basicaly_ignore )
{
	struct local {
		~local() {kiSUtil::switchCurDirToExeDir(); } // Avoid holding a directory lock
	} _;

	//-- Store file name list in Archiver Manager
	if( 0 == m_arcMan.set_files( files ) )
		return;

	//-- Working variables
	enum { unknown, melt, compress }
			whattodo = unknown;
	bool	ctrl_mlt = keyPushed( VK_CONTROL );
	bool	ctrl_cmp = ctrl_mlt;
	bool	alt      = keyPushed( VK_MENU ) || keyPushed( VK_RWIN ) || keyPushed( VK_LWIN );
	const char *cmptype=NULL, *method=NULL;
	kiPath  destdir;
	kiStr tmp(300);

	//-- Parse command-line options (if any)
	if( opts )
		for( unsigned int i=0; i!=opts->len(); i++ )
			switch( (*opts)[i][1] )
			{
			case 'a':	if( !basicaly_ignore )
			case 'A':		whattodo = compress;	break;

			case 'x':	if( !basicaly_ignore )
			case 'X':		whattodo = melt;		break;

			case 'd':	if( !basicaly_ignore )
			case 'D':		destdir = (*opts)[i]+2;	break;

			case 'w':	if( !basicaly_ignore )
			case 'W':		alt = true;				break;

			case 't':	if( !basicaly_ignore )
			case 'T':		cmptype = (*opts)[i]+2;	break;

			case 'm':	if( !basicaly_ignore )
			case 'M':		method = (*opts)[i]+2;	break;

			case 'c':	if( !basicaly_ignore ) {
			case 'C':		if((*opts)[i][2]!='x') ctrl_cmp = true;
							if((*opts)[i][2]!='a') ctrl_mlt = true;
						break;}
			}

	//-- Load-INI ( operation mode settings )
	m_cnfMan.load( Mode );

	//-- Decide whether to compress or extract. Flow is as follows.
	//
	// - If compression is specified on the command line, always compress.
	//
	// - Otherwise, first get Noah's operation mode.
	//   m0:compress-only  m1:compress-preferred  m2:extract-preferred  m3:extract-only
	//   If extraction is specified on the command line, use m3.
	//   If not specified, read from m_cnfMan.
	//
	// - Unconditionally compress if m0, or if m1 and multiple files.
	//
	// - Otherwise, try to assign an extraction routine.
	//   For modes other than m3, any assignment failure falls back to compression.
	//   For m3, if nothing can be assigned, report an error and exit.

	if( whattodo != compress )
	{
		int mode = 3;
		if( whattodo != melt )
			mode = m_cnfMan.mode();

		if( mode==0 || ( mode==1 && m_arcMan.file_num()>=2 ) )
			whattodo = compress;
		else
		{
			//-- Try assigning extraction routine
			bool suc = m_arcMan.map_melters( mode );
			if( suc )
				whattodo = melt;
			else
			{
				if( mode != 3 )
					whattodo = compress;
				else
				{
					//-- Extract-only mode but extraction is impossible!!
					msgBox( tmp.loadRsrc(IDS_M_ERROR) );
					return;
				}
			}
		}
	}

	if( whattodo == melt )
	{
		//-- Extraction settings already loaded by m_cnfMan.init()...

		if( destdir.len()==0 )
		{
			//-- Get extraction destination directory
			if( m_cnfMan.mdirsm() )
				if( is_writable_dir(m_arcMan.get_basepath()) )
					destdir = m_arcMan.get_basepath();
			if( destdir.len()==0 )
				destdir = m_cnfMan.mdir();
		}

		//-- Extract
		if( ctrl_mlt )	m_arcMan.do_listing( destdir );
		else {
			ProcessNumLimitZone zone( mycnf().multiboot_limit(), "LimitterForNoahAtKmonosNet" );
			m_arcMan.do_melting( destdir );
		}
	}
	else
	{
		//-- Load-INI( compression settings )
		m_cnfMan.load( Compress );

		if( destdir.len()==0 )
		{
			//-- Get compression destination directory
			if( m_cnfMan.cdirsm() )
				if( is_writable_dir(m_arcMan.get_basepath()) )
					destdir = m_arcMan.get_basepath();
			if( destdir.len()==0 )
				destdir = m_cnfMan.cdir();
		}
		if( !cmptype ) cmptype = m_cnfMan.cext();
		else if( !method ) method = "";
		if( !method  ) method  = m_cnfMan.cmhd();

		//-- Assign compression routine
		if( !m_arcMan.map_compressor( cmptype, method, ctrl_cmp ) )
		{
			//-- Incompressible format!!
			msgBox( tmp.loadRsrc(IDS_C_ERROR) );
			return;
		}

		//-- Compress
		ProcessNumLimitZone zone( mycnf().multiboot_limit(), "LimitterForNoahAtKmonosNet" );
		m_arcMan.do_compressing( destdir, alt );
	}
}

//----------------------------------------------//
//----------------- Misc utilities -----------------//
//----------------------------------------------//

// from= 0:normal 1:melt 2:compress
void CNoahApp::open_folder( const kiPath& path, int from )
{
	if( from==1 || from==2 ) //-- Notify Shell of update
		::SHChangeNotify( SHCNE_UPDATEDIR, SHCNF_PATH, (const void*)(const char*)path, NULL );

	//-- Don't open if it's the desktop
	kiPath dir(path), tmp(kiPath::Dsk,false);
	dir.beBackSlash(false), dir.beShortPath(), tmp.beShortPath();

	if( !tmp.equalsIgnoreCase( dir ) )
	{
		//-- Load-INI( folder open settings )
		m_cnfMan.load( OpenDir );
		if( (from==1 && !m_cnfMan.modir())
		 || (from==2 && !m_cnfMan.codir()) )
			return;
		
		char cmdline[1000];
		wsprintf( cmdline, m_cnfMan.openby(), (const char*)dir );
		::WinExec( cmdline, SW_SHOWDEFAULT );
	}
}

// Create and return a system-unique temporary folder
void CNoahApp::get_tempdir( kiPath& tmp )
{
	char buf[MAX_PATH];

	if( m_tmpDir.len()==0 )
	{
		::GetTempFileName( kiPath( kiPath::Tmp ), "noa", 0, buf );
		::DeleteFile( buf );
		m_tmpDir = buf;
		m_tmpDir.beBackSlash( true );
		m_tmpDir.mkdir();
		m_tmpID = ::GetCurrentProcessId();
	}

	::GetTempFileName( m_tmpDir, "noa", m_tmpID++, buf );
	::DeleteFile( buf );
	tmp = buf;
	tmp.beBackSlash( true );
	tmp.mkdir();
}
