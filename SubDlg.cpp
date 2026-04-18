
#include "stdafx.h"
#include "NoahApp.h"
#include "SubDlg.h"

int CArcViewDlg::st_nLife;

// Subclass proc for the ListView header: gray background + 1px separator line
static LRESULT CALLBACK HeaderSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WNDPROC oldProc = (WNDPROC)::GetProp(hwnd, "NHdrProc");
	if( msg == WM_ERASEBKGND )
	{
		HDC hdc = (HDC)wp;
		RECT rc; ::GetClientRect(hwnd, &rc);
		HBRUSH br = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
		::FillRect(hdc, &rc, br);
		::DeleteObject(br);
		return TRUE;
	}
	LRESULT r = ::CallWindowProc(oldProc, hwnd, msg, wp, lp);
	if( msg == WM_PAINT )
	{
		RECT rc; ::GetClientRect(hwnd, &rc);
		HDC hdc = ::GetDC(hwnd);
		HPEN pen = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
		HPEN oldPen = (HPEN)::SelectObject(hdc, pen);
		::MoveToEx(hdc, 0, rc.bottom - 1, NULL);
		::LineTo(hdc, rc.right, rc.bottom - 1);
		::SelectObject(hdc, oldPen);
		::DeleteObject(pen);
		::ReleaseDC(hwnd, hdc);
	}
	return r;
}

BOOL CArcViewDlg::onInit()
{
	kiStr str;
	kiPath path;
	SHFILEINFO sfi,lfi;
	HIMAGELIST hImS,hImL;
	kiListView ctrl( this, IDC_FILELIST );

	//-- Mark that one dialog has been created
	hello();

	//-- Center and bring to front
	setCenter( hwnd(), app()->mainhwnd() );
	setFront( hwnd() );

	//-- Icon
	path = m_fname.basedir, path += m_fname.sname;
	hImS = (HIMAGELIST)::SHGetFileInfo( path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_SMALLICON );
	hImL = (HIMAGELIST)::SHGetFileInfo( path, 0, &lfi, sizeof(lfi), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_LARGEICON );
	sendMsg( WM_SETICON, ICON_BIG,   (LPARAM)lfi.hIcon );
	sendMsg( WM_SETICON, ICON_SMALL, (LPARAM)sfi.hIcon );

	//-- Title
	sendMsg( WM_SETTEXT, 0, (LPARAM)kiPath(m_fname.lname).name() );

	//-- Extraction destination (display as long path even if m_ddir is short path)
	{
		char longdir[MAX_PATH];
		const char* disp = (::GetLongPathName((const char*)m_ddir, longdir, MAX_PATH) > 0)
			? longdir : (const char*)m_ddir;
		sendMsgToItem( IDC_DDIR, WM_SETTEXT, 0, (LPARAM)disp );
	}

	//-- Monospace font for raw archiver output
	{
		LOGFONT lf;
		::GetObject( (HFONT)sendMsg(WM_GETFONT), sizeof(lf), &lf );
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		lf.lfCharSet = DEFAULT_CHARSET;
		ki_strcpy( lf.lfFaceName, "Consolas" );
		m_hFont = ::CreateFontIndirect( &lf );
	}
	::SendMessage( item(IDC_FILELIST), WM_SETFONT, (WPARAM)m_hFont, TRUE );
	m_hHeader = (HWND)sendMsgToItem( IDC_FILELIST, LVM_GETHEADER, 0, 0 );
	if( m_hHeader )
	{
		::SendMessage( m_hHeader, WM_SETFONT, (WPARAM)m_hFont, TRUE );

		// Disable visual styles on the header → classic gray (COLOR_BTNFACE) background
		typedef HRESULT (WINAPI *FnSWT)(HWND, LPCWSTR, LPCWSTR);
		HMODULE hUx = ::LoadLibrary("uxtheme.dll");
		if( hUx )
		{
			FnSWT pfn = (FnSWT)::GetProcAddress(hUx, "SetWindowTheme");
			if( pfn ) pfn(m_hHeader, L"", L"");
			::FreeLibrary(hUx);
		}

		// Subclass to add WM_ERASEBKGND (gray fill) and WM_PAINT (separator line)
		WNDPROC old = (WNDPROC)(LONG_PTR)::GetWindowLongPtr(m_hHeader, GWLP_WNDPROC);
		::SetProp(m_hHeader, "NHdrProc", (HANDLE)old);
		::SetWindowLongPtr(m_hHeader, GWLP_WNDPROC, (LONG_PTR)HeaderSubclassProc);
	}

	//-- List
	if( !m_pArc->list( m_fname, m_files ) || m_files.len()==0 )
	{
		m_bAble = false;
		ctrl.insertColumn( 0, "", 510 );
		ctrl.insertItem( 0, str.loadRsrc(IDS_NOLIST) );
	}
	else
	{
		m_bAble = ( 0 != (m_pArc->ability() & aMeltEach) );

		// First entry is the column header captured from archiver output (isfile=false)
		const char* colHeader = "";
		unsigned int dataStart = 0;
		if( m_files.len() > 0 && !m_files[0].isfile )
		{
			colHeader = m_files[0].rawline;
			dataStart = 1;
		}
		ctrl.insertColumn( 0, colHeader, 800 );

		for( unsigned int i=dataStart, k=0; i!=m_files.len(); i++ )
			if( m_files[i].isfile )
				ctrl.insertItem( k++, m_files[i].rawline, (LPARAM)(&m_files[i]) );

		//-- Register drag & drop format
		FORMATETC fmt;
		fmt.cfFormat = CF_HDROP;
		fmt.ptd      = NULL;
		fmt.dwAspect = DVASPECT_CONTENT;
		fmt.lindex   = -1;
		fmt.tymed    = TYMED_HGLOBAL;
		addFormat( fmt );
	}

	//-- Info --
	char tmp[255];
	kiStr full_filename = m_fname.basedir + m_fname.lname;
	unsigned int fileCount = 0;
	for( unsigned int i=0; i!=m_files.len(); i++ )
		if( m_files[i].isfile ) fileCount++;
	wsprintf( tmp, kiStr().loadRsrc(IDS_ARCVIEW_MSG),
		fileCount,
		0,
		(const char*)m_pArc->arctype_name(full_filename)
	);
	sendMsgToItem( IDC_STATUSBAR, WM_SETTEXT, 0, (LPARAM)tmp );

	if( !m_bAble )
	{
		static const UINT items[] = { IDC_SELECTINV,IDC_REF,IDC_MELTEACH,IDC_SHOW,IDC_DDIR };
		for( int i=0; i!=sizeof(items)/sizeof(UINT); i++ )
			::EnableWindow( item(items[i]), FALSE );
	}

	return FALSE;
}

bool CArcViewDlg::onOK()
{
	setdir();
	m_pArc->melt( m_fname, m_ddir );
	myapp().open_folder( m_ddir, 1 );
	kiSUtil::switchCurDirToExeDir(); // Just to be safe
	return onCancel();
}

bool CArcViewDlg::onCancel()
{
	::SetCurrentDirectory( m_fname.basedir );
	m_tdir.remove();
	if( kiSUtil::exist(m_tdir) )
	{
		kiStr tmp(600);
		if( IDNO==app()->msgBox( tmp.loadRsrc(IDS_EXECUTING), NULL, MB_YESNO|MB_DEFBUTTON2 ) )
			return false;
	}

	kiListView(this,IDC_FILELIST).setImageList( NULL, NULL );
	if( m_hHeader )
	{
		WNDPROC old = (WNDPROC)::GetProp(m_hHeader, "NHdrProc");
		if( old ) ::SetWindowLongPtr(m_hHeader, GWLP_WNDPROC, (LONG_PTR)old);
		::RemoveProp(m_hHeader, "NHdrProc");
		m_hHeader = NULL;
	}
	if( m_hFont ) { ::DeleteObject(m_hFont); m_hFont=NULL; }
	byebye();
	return true;
}

bool CArcViewDlg::giveData( const FORMATETC& fmt, STGMEDIUM* stg, bool firstcall )
{
	if( firstcall )
		if( 0x8000<=m_pArc->melt( m_fname, m_tdir, &m_files ) )
			return false;

	unsigned int i;
	BOOL fWide = (app()->osver().dwPlatformId==VER_PLATFORM_WIN32_NT);
	kiArray<kiPath> lst;
	kiPath tmp;
	int flen = 0;
	wchar_t wbuf[600];

	for( i=0; i!=m_files.len(); i++ )
		if( m_files[i].selected )
		{
			tmp = m_tdir;
			tmp += m_files[i].inf.szFileName;

			lst.add( tmp );
			if( fWide )
				flen += (::MultiByteToWideChar( CP_ACP, 0, tmp, -1, wbuf, 600 )+1)*2;
			else
				flen += (tmp.len()+1);
		}

	HDROP hDrop = (HDROP)::GlobalAlloc( GHND, sizeof(DROPFILES)+flen+1 );

	DROPFILES* dr = (DROPFILES*)::GlobalLock( hDrop );
	dr->pFiles = sizeof(DROPFILES);
	dr->pt.x   = dr->pt.y = 0;
	dr->fNC    = FALSE;
	dr->fWide  = fWide;

	char* buf = (char*)(&dr[1]);
	for( i=0; i!=lst.len(); i++ )
	{
		if( fWide )
		{
			flen = ::MultiByteToWideChar( CP_ACP, 0, lst[i], -1, wbuf, 600 );
			ki_memcpy( buf, wbuf, flen*2 );
			for( int k=0; k!=flen; k++ )
				if( ((wchar_t*)buf)[k] == '/' )
					((wchar_t*)buf)[k] = '\\';
			buf += flen*2;
		}
		else
		{
			ki_strcpy( buf,lst[i] );
			for( int k=0; k!=lst[i].len(); k++ )
				if( buf[k] == '/' )
					buf[k] = '\\';
			buf += lst[i].len() + 1;
		}
	}
	*buf=0;
	if( fWide )
		buf[1]='\0';

	::GlobalUnlock( hDrop );

	stg->hGlobal        = hDrop;
	stg->tymed          = TYMED_HGLOBAL;
	stg->pUnkForRelease = NULL;
	return true;
}

BOOL CALLBACK CArcViewDlg::proc( UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	//-- Specify main window ---------------------
	case WM_ACTIVATE:
		if( LOWORD(wp)==WA_ACTIVE || LOWORD(wp)==WA_CLICKACTIVE )
		{
			app()->setMainWnd( this );
			return TRUE;
		}
		break;

	//-- Resize-related processing ---------------------
	case WM_GETMINMAXINFO:
		{
			RECT self,child;
			::GetWindowRect( hwnd(), &self );
			::GetWindowRect( item(IDC_REF), &child );
			POINT& sz = ((MINMAXINFO*)lp)->ptMinTrackSize;
			sz.x = child.right - self.left + 18;
			sz.y = child.bottom - self.top + 100;
		}
		return TRUE;
	case WM_SIZE:
		if( wp!=SIZE_MAXHIDE && wp!=SIZE_MINIMIZED )
		{
			RECT self,ref,child,sbar;
			::GetWindowRect( hwnd(), &self );
			::GetWindowRect( item(IDC_REF), &ref );
			::GetWindowRect( item(IDC_FILELIST), &child );
			::GetClientRect( item(IDC_STATUSBAR), &sbar );

			::SetWindowPos( item(IDC_FILELIST), NULL, 0, 0,
				LOWORD(lp),
				(self.bottom-ref.bottom)-(child.top-ref.bottom)
				-(sbar.bottom-sbar.top)-10,
				SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER );

			::GetClientRect( hwnd(), &self );
			::SetWindowPos( item(IDC_STATUSBAR), NULL, sbar.left,
				self.bottom - (sbar.bottom-sbar.top),
				0, 0, SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER );
		}
		break;

	case WM_NOTIFY:
		if( wp==IDC_FILELIST && m_bAble )
		{
			NMHDR* phdr=(NMHDR*)lp;
			if( phdr->code==LVN_BEGINDRAG || phdr->code==LVN_BEGINRDRAG )
			{
				if( setSelection() )
					kiDropSource::DnD( this, DROPEFFECT_COPY );
				return TRUE;
			}
			else if( phdr->code==NM_DBLCLK )
				sendMsg( WM_COMMAND, IDC_SHOW );
			else if( phdr->code==NM_RCLICK )
			{
				if( setSelection() )
					DoRMenu();
			}
		}
		break;

	case WM_COMMAND:
		switch( LOWORD(wp) )
		{
		case IDC_SELECTINV: // Invert selection
			{
				LVITEM item;
				item.mask = LVIF_STATE;
				item.stateMask = LVIS_SELECTED;
				int j,m = static_cast<int>(sendMsgToItem( IDC_FILELIST, LVM_GETITEMCOUNT ));
				for( j=0; j!=m; j++ )
				{
					item.state = ~static_cast<UINT>(sendMsgToItem( IDC_FILELIST, LVM_GETITEMSTATE, j, LVIS_SELECTED ));
					sendMsgToItem( IDC_FILELIST, LVM_SETITEMSTATE, j, (LPARAM)&item );
				}
				::SetFocus( this->item(IDC_FILELIST) );
			}
			return TRUE;

		case IDC_REF: // Set extraction destination
			kiSUtil::getFolderDlgOfEditBox( item(IDC_DDIR), hwnd(), kiStr().loadRsrc(IDS_CHOOSEDIR) );
			return TRUE;

		case IDC_MELTEACH: // Partial extraction
			if( setSelection() )
			{
				setdir();
				int result = m_pArc->melt( m_fname, m_ddir, &m_files );
				if( result<0x8000 )
					myapp().open_folder( m_ddir, 1 );
				else if( result != 0x8020 )
				{
					char str[255];
					wsprintf( str, "%s\nError No: [%x]",
						(const char*)kiStr().loadRsrc( IDS_M_ERROR ), result );
					app()->msgBox( str );
				}
				kiSUtil::switchCurDirToExeDir(); // Just to be safe
			}
			return TRUE;

		case IDC_SHOW: // Show
			if( setSelection() )
			{
				int assocCnt = hlp_cnt_check();
				if( 0x8000 > m_pArc->melt( m_fname, m_tdir, &m_files ) )
				{
					if( assocCnt != -1 )
						m_files[assocCnt].selected = false;
					for( unsigned i=0; i!=m_files.len(); i++ )
						if( m_files[i].selected )
						{
							kiPath tmp(m_tdir);
							char yen[MAX_PATH];
							ki_strcpy( yen, m_files[i].inf.szFileName );
							for( char* p=yen; *p; p=kiStr::next(p) )
								if( *p=='/' )
									*p = '\\';
							tmp += yen;
							::ShellExecute( hwnd(), NULL, tmp, NULL, m_tdir, SW_SHOWDEFAULT );
						}
				}
				kiSUtil::switchCurDirToExeDir(); // Just to be safe
			}
			return TRUE;
		}
	}
	return FALSE;
}

int CArcViewDlg::hlp_cnt_check()
{
	// Whether the first selected file is .hlp
	unsigned int i=0;
	for( ; i!=m_files.len(); i++ )
		if( m_files[i].selected )
			break;
	if( i==m_files.len() )
		return -1;
	ptrdiff_t x = kiPath::ext(m_files[i].inf.szFileName)-m_files[i].inf.szFileName;
	if( 0!=ki_strcmpi( "hlp", m_files[i].inf.szFileName+x ) )
		return -1;

	// .cnt filename
	char cntpath[FNAME_MAX32];
	ki_strcpy( cntpath, m_files[i].inf.szFileName );
	cntpath[x]='c', cntpath[x+1]='n', cntpath[x+2]='t';

	// Also temporarily select .cnt
	for( i=0; i!=m_files.len(); i++ )
		if( 0==ki_strcmpi( cntpath, m_files[i].inf.szFileName ) )
		{
			if( m_files[i].selected )
				return -1;
			m_files[i].selected = true;
			return i;
		}
	return -1;
}

int CALLBACK CArcViewDlg::lv_compare( LPARAM p1, LPARAM p2, LPARAM type )
{
	bool rev = false;
	if( type>=10000 )
		rev=true, type-=10000;
	int ans = 0;

	INDIVIDUALINFO *a1=&((arcfile*)p1)->inf, *a2=&((arcfile*)p2)->inf;
	switch( type )
	{
	case 0: //NAME
		ans = ::lstrcmp( kiPath::name(a1->szFileName),
			             kiPath::name(a2->szFileName) );
		break;
	case 1: //SIZE
		ans = (signed)a1->dwOriginalSize - (signed)a2->dwOriginalSize;
		break;
	case 2: //DATE,TIME
		ans = (signed)a1->wDate - (signed)a2->wDate;
		if( ans==0 )
			ans = (signed)a1->wTime - (signed)a2->wTime;
		break;
	case 3:{//RATIO
		int cr1, cr2;
		if( a1->dwOriginalSize==0 )        cr1=100;
		else if( a1->dwCompressedSize==0 ) cr1=-1;
		else cr1 = (a1->dwCompressedSize*100)/(a1->dwOriginalSize);
		if( a2->dwOriginalSize==0 )        cr2=100;
		else if( a2->dwCompressedSize==0 ) cr2=-1;
		else cr2 = (int)((((__int64)a2->dwCompressedSize)*100)/(a2->dwOriginalSize));
		ans = cr1 - cr2;
		}break;
	case 4: //METHOD
		ans = ::lstrcmp( a1->szMode, a2->szMode );
		break;
	case 5:{//PATH
		kiPath pt1(a1->szFileName), pt2(a2->szFileName);
		pt1.beDirOnly(), pt2.beDirOnly();
		ans = ::lstrcmp( pt1, pt2 );
		}break;
	}

	return rev ? -ans : ans;
}

void CArcViewDlg::DoSort( int col )
{
	WPARAM p = col + (m_bSmallFirst[col] ? 0 : 10000);
	sendMsgToItem( IDC_FILELIST, LVM_SORTITEMS, p, (LPARAM)lv_compare );
	m_bSmallFirst[col] = !m_bSmallFirst[col];
}

void CArcViewDlg::GenerateDirMenu( HMENU m, int& id, StrArray* sx, const kiPath& pth )
{
	// List folder contents
	kiFindFile ff;
	ff.begin( kiPath(pth)+="*" );
	for( WIN32_FIND_DATA fd; ff.next(&fd); )
		if( fd.cFileName[0]!='.'
		 && !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) )
		{
			kiPath fullpath(pth); fullpath+=fd.cFileName;
			const int pID=id;
			MENUITEMINFO mi = { sizeof(MENUITEMINFO) };

			if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// Recursively
				mi.fMask = MIIM_SUBMENU | 0x00000040;// (MIIM_STRING)
				mi.hSubMenu = ::CreatePopupMenu();
				GenerateDirMenu( mi.hSubMenu, id, sx,
					kiPath(kiPath(fullpath)+="\\") );
			}
			else
			{
				const char* ext = kiPath::ext(fd.cFileName);
				if( ::lstrlen(ext) > 4 ) continue;
				if( 0==::lstrcmpi(ext,"lnk") )
					*const_cast<char*>(ext-1) = '\0';
				mi.fMask = MIIM_ID | 0x00000040;// (MIIM_STRING)
				mi.wID = id++;
				sx->add( fullpath );
			}

			mi.dwTypeData = const_cast<char*>((const char*)fd.cFileName);
			mi.cch        = ::lstrlen(fd.cFileName);
			::InsertMenuItem( m, pID, FALSE, &mi );
		}
}

void CArcViewDlg::DoRMenu()
{
	// Create menu
	HMENU m = ::CreatePopupMenu();
	POINT pt; ::GetCursorPos( &pt );
	const int IDSTART = 128;

	// List folder contents and add to menu
	int id = IDSTART;
	StrArray lst;
	GenerateDirMenu( m, id, &lst, kiPath(CSIDL_SENDTO) );

	// Show menu
	id = ::TrackPopupMenu( m,
		TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,
		pt.x, pt.y, 0, hwnd(), NULL );
	::DestroyMenu( m );

	// Result processing
	if( id != 0 )
	{
		kiStr cmd;
		if( 0x8000>m_pArc->melt( m_fname, m_tdir, &m_files ) )
		{
			for( UINT i=0; i!=m_files.len(); i++ )
				if( m_files[i].selected )
				{
					cmd += "\"";
					cmd += m_tdir;
					const char* buf = m_files[i].inf.szFileName;
					for( int k=0; buf[k]; ++k )
						cmd += ( buf[k]=='/' ? '\\' : buf[k] );
					cmd += "\" ";
				}
			ShellExecute(hwnd(),NULL,lst[id-IDSTART],cmd,NULL,SW_SHOW);
		}
	}
}
