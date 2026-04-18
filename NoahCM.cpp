// NoahCM.cpp
//-- CNoahConfigManager -- save / load / modify the setting of 'Noah' --

#include "stdafx.h"
#include "resource.h"
#include "NoahApp.h"
#include "NoahCM.h"
#include <algorithm>

//----------------------------------------------//
//------------- INI file setup etc. ------------//
//----------------------------------------------//

void CNoahConfigManager::init()
{
	//-- Clear the settings-loaded flag
	m_Loaded = 0;

	//-- Set INI file name
	char usr[256];
	DWORD siz=sizeof(usr);
	if( !::GetUserName( usr, &siz ) )
		ki_strcpy( usr, "Default" );
	m_Ini.setFileName( "Noah.ini" );
	m_Ini.setSection( usr );

	//-- Pre-load all extraction settings
	load( Melt );
}

//----------------------------------------------//
//------------- Settings load & save -----------//
//----------------------------------------------//

void CNoahConfigManager::load( loading_flag what )
{
	if( (what & Mode) && !(m_Loaded & Mode) ) //----------- Mode
	{
		m_Mode     = m_Ini.getInt( "Mode", 2 ) & 3;
		m_MiniBoot = m_Ini.getBool( "MiniBoot", false );
		m_OneExt   = m_Ini.getBool( "OneExt", false );
		m_ZeroExt  = m_Ini.getBool( "NoExt", false );
		m_MbLim    = (std::max)( 1, m_Ini.getInt( "MultiBootLimit", 4 ) );
		m_OldVer   = m_Ini.getBool( "OldAbout", false );
	}
	if( (what & Melt) && !(m_Loaded & Melt) ) //----------- Extraction
	{
		const char* x = m_Ini.getStr( "MDir", kiPath( kiPath::Dsk ) );
		m_MDirSm = (*x=='@');
		m_MDir   = (*x=='@') ? x+1 : x;
		const int m = m_Ini.getInt( "MkDir", 2 );
		m_MNoNum = ( m>=16 );
		m_MkDir  = ( m&3 );
		m_Kill   = m_Ini.getStr( "Kill", "" );
	}
	if( (what & Compress) && !(m_Loaded & Compress) ) //--- Compression
	{
		const char* x = m_Ini.getStr( "CDir", kiPath( kiPath::Dsk ) );
		m_CDirSm = (*x=='@');
		m_CDir   = (*x=='@') ? x+1 : x;
		m_CExt = m_Ini.getStr( "CExt", "zip" );
		m_CMhd = m_Ini.getStr( "CMhd", "7-zip" );
	}
	if( (what & OpenDir) && !(m_Loaded & OpenDir) ) //------ Folder open
	{
		m_MODir = m_Ini.getBool( "MODir", true );
		m_CODir = m_Ini.getBool( "CODir", true );
		m_OpenBy = m_Ini.getStr( "OpenBy", kiPath(kiPath::Win)+"explorer.exe \"%s\"" );
	}

	m_Loaded |= what;
}

void CNoahConfigManager::save()
{
	kiStr tmp;

	//-- Mode
	m_Ini.putInt( "Mode", m_Mode );
	//-- Extraction
	tmp = m_MDirSm ? "@" : "", tmp+= m_MDir;
	m_Ini.putStr( "MDir", tmp );
	m_Ini.putInt( "MkDir", m_MkDir+(m_MNoNum?16:0) );
	//-- Compression
	tmp = m_CDirSm ? "@" : "", tmp+= m_CDir;
	m_Ini.putStr( "CDir", tmp );
	m_Ini.putStr( "CExt", m_CExt );
	m_Ini.putStr( "CMhd", m_CMhd );
	//-- Folder open
	m_Ini.putBool("MODir", m_MODir );
	m_Ini.putBool("CODir", m_CODir );
}

void CNoahConfigManager::dialog()
{
	CNoahConfigDialog dlg;
	dlg.createModeless( NULL );

	app()->setMainWnd( &dlg );

	if( dlg.isAlive() )
		kiWindow::msgLoop();
}

//----------------------------------------------//
//--------------- Dialog related ---------------//
//----------------------------------------------//

///////// Initialize /////////////

	CNoahConfigDialog::CCmprPage::CCmprPage() : kiPropSheetPage( IDD_CMPCFG ) {}
	CNoahConfigDialog::CMeltPage::CMeltPage() : kiPropSheetPage( IDD_MLTCFG ) {}
	CNoahConfigDialog::CInfoPage::CInfoPage() : kiPropSheetPage( IDD_INFCFG ) {}

CNoahConfigDialog::CNoahConfigDialog()
{
	//-- [icon] Noah properties
	m_Header.dwFlags |= PSH_PROPTITLE | PSH_USEICONID;
	m_Header.pszIcon = MAKEINTRESOURCE( IDI_MAIN );
	m_Header.pszCaption = "Noah";

	//-- Set accelerator
	loadAccel( IDR_ACCEL );

	//-- Add pages
	m_Pages.add( new CCmprPage );
	m_Pages.add( new CMeltPage );
	m_Pages.add( new CInfoPage );
}

BOOL CNoahConfigDialog::onInit()
{
	//-- Enable DnD, bring to front
	::DragAcceptFiles( hwnd(), TRUE );
	setFront( hwnd() );
	return FALSE;
}

///////// Commands /////////////

void CNoahConfigDialog::onCommand( UINT id )
{
	//-- Trap accelerator
	if( id == IDA_HELP )		onHelp();
	else if( id == IDA_MYDIR )	myapp().open_folder( kiPath( kiPath::Exe ) );
}

void CNoahConfigDialog::onHelp()
{
	kiPath exepos( kiPath::Exe );

	//-- Open Noah.html from the same directory as the exe
	kiPath hlp(exepos); hlp+="Noah.html";
	if( kiSUtil::exist(hlp) )
		::ShellExecute( hwnd(), NULL, hlp, NULL, NULL, SW_MAXIMIZE );
	else
	{
		//-- Fall back to Noah.txt
		hlp=exepos; hlp+="Noah.txt";
		if( kiSUtil::exist(hlp) )
			::ShellExecute( hwnd(), NULL, hlp, NULL, NULL, SW_SHOWDEFAULT );
	}
}

void CNoahConfigDialog::onDrop( HDROP hdrop )
{
	//-- Drag & drop onto the dialog: commit settings first
	sendOK2All();

	//-- Hide while processing
	::ShowWindow( hwnd(), SW_HIDE );

	char str[MAX_PATH];
	StrArray reallist;
	cCharArray dummy;

	unsigned int i;
	unsigned long max = ::DragQueryFile( hdrop, 0xffffffff, NULL, 0 );
	for( i=0; i!=max; i++ )
	{
		::DragQueryFile( hdrop, i, str, MAX_PATH );
		reallist.add( kiStr(str) );
	}
	for( i=0; i!=max; i++ )
		dummy.add( (const char*)reallist[i] );
	myapp().do_files( dummy, NULL );

	//-- Restore
	::DragFinish( hdrop );
	::ShowWindow( hwnd(), SW_SHOW );
}

///////// Exit processing /////////////

void CNoahConfigDialog::shift_and_button()
{
	if( app()->keyPushed( VK_SHIFT ) )
	{
		app()->setMainWnd( NULL );
		myapp().do_cmdline();
	}
}

bool CNoahConfigDialog::onOK()
{
	onApply();
	::PostQuitMessage( 0 );
	return true;
}

void CNoahConfigDialog::onApply()
{
	mycnf().save();
	shift_and_button();
}

bool CNoahConfigDialog::onCancel()
{
	sendOK2All();
	::ShowWindow( hwnd(), SW_HIDE );
	shift_and_button();
	::PostQuitMessage( 0 );
	return true;
}

///////// Shared code for compression/extraction settings /////////////

static void dirinit( kiDialog* dlg, bool same, bool open, const char* dir )
{
	dlg->sendMsgToItem( same ? IDC_DDIR1 : IDC_DDIR2 , BM_SETCHECK, TRUE );
	if( open )
		dlg->sendMsgToItem( IDC_ODIR , BM_SETCHECK, TRUE );
	dlg->sendMsgToItem( IDC_DDIR , WM_SETTEXT , 0, (LPARAM)dir );
}

static void dirok( kiDialog* dlg, bool& same, bool& open, kiPath& dir )
{
	same = ( BST_CHECKED==dlg->sendMsgToItem( IDC_DDIR1, BM_GETCHECK ) );
	open = ( BST_CHECKED==dlg->sendMsgToItem( IDC_ODIR, BM_GETCHECK ) );
	static char str[MAX_PATH];
	dlg->sendMsgToItem( IDC_DDIR, WM_GETTEXT, MAX_PATH, (LPARAM)str );
	dir = str;
}

static bool dirdlg( kiDialog* dlg, UINT msg, WPARAM wp )
{
	if( msg==WM_COMMAND && LOWORD(wp)==IDC_REF )
	{
		kiSUtil::getFolderDlgOfEditBox(
			dlg->item(IDC_DDIR), dlg->hwnd(), kiStr().loadRsrc(IDS_CHOOSEDIR) );
		return true;
	}
	return false;
}

///////// Compression settings /////////////

BOOL CNoahConfigDialog::CCmprPage::onInit()
{
	// Compression destination folder
	dirinit( this, mycnf().cdirsm(), mycnf().codir(), mycnf().cdir() );

	// Operation mode
	sendMsgToItem( IDC_MODE1 + mycnf().mode(), BM_SETCHECK, TRUE );

	// Compression format
	correct( mycnf().cext(), true );
	int ind = static_cast<int>(sendMsgToItem( IDC_CMPMHD, CB_FINDSTRINGEXACT, -1, (LPARAM)(const char*)mycnf().cmhd() ));
	if( ind!=CB_ERR )
		sendMsgToItem( IDC_CMPMHD, CB_SETCURSEL, ind );

	// Tooltip
	m_tooltip = ::CreateWindowEx(
		0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd(), NULL, app()->inst(), NULL );
	SetUpToolTip();

	::SetFocus(hwnd());
	return TRUE;
}

bool CNoahConfigDialog::CCmprPage::onOK()
{
	// Compression destination folder
	dirok( this, mycnf().m_CDirSm, mycnf().m_CODir, mycnf().m_CDir );

	// Operation mode
	for( int i=0; i!=4; i++ )
		if( BST_CHECKED==sendMsgToItem( IDC_MODE1 + i, BM_GETCHECK ) )
			{ mycnf().m_Mode = i; break; }

	// Compression format
	char str[200]="";
	sendMsgToItem( IDC_CMPEXT, CB_GETLBTEXT, sendMsgToItem( IDC_CMPEXT, CB_GETCURSEL ), (LPARAM)str );
	if( *str )
	{
		mycnf().m_CExt = str;
		sendMsgToItem( IDC_CMPMHD, CB_GETLBTEXT, sendMsgToItem( IDC_CMPMHD, CB_GETCURSEL ), (LPARAM)str );
		mycnf().m_CMhd = str;
	}

	onCancel(); // cleanup
	return true;
}

bool CNoahConfigDialog::CCmprPage::onCancel()
{
	// cleanup
	::DestroyWindow( m_tooltip );
	return true;
}

void CNoahConfigDialog::CCmprPage::SetUpToolTip()
{
	char ext[200]="";
	sendMsgToItem( IDC_CMPEXT, CB_GETLBTEXT, sendMsgToItem( IDC_CMPEXT, CB_GETCURSEL ), (LPARAM)ext );
	char mhd[200]="";
	sendMsgToItem( IDC_CMPMHD, CB_GETLBTEXT, sendMsgToItem( IDC_CMPMHD, CB_GETCURSEL ), (LPARAM)mhd );

	TOOLINFO ti = {sizeof(TOOLINFO)};
	ti.uFlags   = TTF_SUBCLASS;
	{
		ti.uId      = 0;
		ti.hwnd     = item(IDC_CMPEXT);
		::GetClientRect( item(IDC_CMPEXT), &ti.rect );
		SendMessage( m_tooltip, TTM_DELTOOL, 0, (LPARAM)&ti );
		if( *ext )
		{
			ti.lpszText = ext;
			SendMessage( m_tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti );
		}
	}
	{
		ti.uId      = 1;
		ti.hwnd     = item(IDC_CMPMHD);
		::GetClientRect( item(IDC_CMPMHD), &ti.rect );
		SendMessage( m_tooltip, TTM_DELTOOL, 0, (LPARAM)&ti );
		if( *mhd )
		{
			ti.lpszText = mhd;
			SendMessage( m_tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti );
		}
	}
}

BOOL CALLBACK CNoahConfigDialog::CCmprPage::proc( UINT msg, WPARAM wp, LPARAM lp )
{
	if( dirdlg( this, msg, wp ) )
		return TRUE;

	if( msg==WM_COMMAND && HIWORD(wp)==CBN_SELCHANGE && LOWORD(wp)==IDC_CMPEXT )
	{
		char str[200]="";
		sendMsgToItem( IDC_CMPEXT, CB_GETLBTEXT, sendMsgToItem( IDC_CMPEXT, CB_GETCURSEL ), (LPARAM)str );
		if( *str )
			correct( str, false );
		SetUpToolTip();
		return TRUE;
	}
	else if( msg==WM_COMMAND && HIWORD(wp)==CBN_SELCHANGE && LOWORD(wp)==IDC_CMPMHD )
	{
		SetUpToolTip();
	}
	return FALSE;
}

void CNoahConfigDialog::CCmprPage::correct( const char* ext, bool first )
{
	cCharArray extl;
	StrArray mhdl;
	int mhdef;
	myarc().get_cmpmethod( ext, mhdef, mhdl, first, &extl );

	if( first )
		for( unsigned int i=0; i!=extl.len(); i++ )
			sendMsgToItem( IDC_CMPEXT, CB_ADDSTRING, 0, (LPARAM)extl[i] );
	sendMsgToItem( IDC_CMPEXT, CB_SELECTSTRING, 0, (LPARAM)ext );

	sendMsgToItem( IDC_CMPMHD, CB_RESETCONTENT );
	for( unsigned int j=0; j!=mhdl.len(); j++ )
		sendMsgToItem( IDC_CMPMHD, CB_ADDSTRING, 0, (LPARAM)(const char*)mhdl[j] );
	sendMsgToItem( IDC_CMPMHD, CB_SETCURSEL, mhdef );
}


///////// Extraction settings /////////////

BOOL CNoahConfigDialog::CMeltPage::onInit()
{
	// Extraction destination folder
	dirinit( this, mycnf().mdirsm(), mycnf().modir(), mycnf().mdir() );

	// Auto folder creation
	if( mycnf().mkdir()!=0 )
		sendMsgToItem( IDC_MKDIR ,BM_SETCHECK, TRUE );
	if( mycnf().mkdir()==1 )
		sendMsgToItem( IDC_MKDIR1,BM_SETCHECK, TRUE );
	if( mycnf().mkdir()==2 )
		sendMsgToItem( IDC_MKDIR2,BM_SETCHECK, TRUE );
	if( mycnf().mnonum() )
		sendMsgToItem( IDC_MKDIR3,BM_SETCHECK, TRUE );
	correct();

	return FALSE;
}

bool CNoahConfigDialog::CMeltPage::onOK()
{
	// Extraction destination folder
	dirok( this, mycnf().m_MDirSm, mycnf().m_MODir, mycnf().m_MDir );

	// Auto folder creation
	mycnf().m_MNoNum = ( BST_CHECKED==sendMsgToItem( IDC_MKDIR3, BM_GETCHECK ) );
	if( BST_CHECKED!=sendMsgToItem( IDC_MKDIR ,BM_GETCHECK ) )
		mycnf().m_MkDir = 0;
	else
	{
		if( BST_CHECKED==sendMsgToItem( IDC_MKDIR1 ,BM_GETCHECK ) )
			mycnf().m_MkDir = 1;
		else if( BST_CHECKED==sendMsgToItem( IDC_MKDIR2 ,BM_GETCHECK ) )
			mycnf().m_MkDir = 2;
		else
			mycnf().m_MkDir = 3;
	}
	return true;
}

BOOL CALLBACK CNoahConfigDialog::CMeltPage::proc( UINT msg, WPARAM wp, LPARAM lp )
{
	if( dirdlg( this, msg, wp ) )
		return TRUE;

	if( msg==WM_COMMAND )
		if( LOWORD(wp)==IDC_MKDIR || LOWORD(wp)==IDC_MKDIR1 )
		{
			correct();
			return TRUE;
		}

	return FALSE;
}

void CNoahConfigDialog::CMeltPage::correct()
{
	BOOL _mk = ( BST_CHECKED==sendMsgToItem( IDC_MKDIR ,BM_GETCHECK ) );
	BOOL _1f = ( BST_CHECKED==sendMsgToItem( IDC_MKDIR1,BM_GETCHECK ) );
	::EnableWindow( ::GetDlgItem(hwnd(),IDC_MKDIR1), _mk );
	::EnableWindow( ::GetDlgItem(hwnd(),IDC_MKDIR2), _mk && !_1f );
	::EnableWindow( ::GetDlgItem(hwnd(),IDC_MKDIR3), _mk );
	if( _1f )
		sendMsgToItem( IDC_MKDIR2, BM_SETCHECK, TRUE );
}

///////// Info page /////////////

BOOL CNoahConfigDialog::CInfoPage::onInit()
{
	kiStr ver;
	myarc().get_version( ver );
	sendMsgToItem( IDC_VERSION, WM_SETTEXT, 0, (LPARAM)(const char*)ver );
	return FALSE;
}
