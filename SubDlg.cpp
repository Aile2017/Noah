
#include "stdafx.h"
#include "NoahApp.h"
#include "SubDlg.h"

int CArcViewDlg::st_nLife;
static const int ARCVIEW_SPLITTER_WIDTH = 3;

// Binary-search a folder path in m_folderPaths using m_folderSorted. Defined
// later in this file; forward-declared here for use inside CArcViewDlg::onInit.
static unsigned int folder_bisect( const StrArray& paths, const kiArray<unsigned int>& sorted,
	const char* buf, bool* found );

static void load_mru_list( StrArray& list )
{
	list.empty();
	for( int i=1; i<=10; i++ )
	{
		char key[8]; wsprintf( key, "MRU%d", i );
		const char* v = mycnf().getStr( key, "" );
		if( v && v[0] )
		{
			bool dup = false;
			for( unsigned int j=0; j<list.len(); j++ )
				if( 0 == ::lstrcmpi( (const char*)list[j], v ) )
				{
					dup = true;
					break;
				}
			if( !dup )
				list.add( kiStr(v) );
		}
	}
}

static void persist_mru_list( const StrArray& list )
{
	for( unsigned int i=0; i<list.len(); i++ )
	{
		char key[8]; wsprintf( key, "MRU%d", i+1 );
		mycnf().putStr( key, (const char*)list[i] );
	}
	for( unsigned int i=list.len(); i<10; i++ )
	{
		char key[8]; wsprintf( key, "MRU%d", i+1 );
		mycnf().putStr( key, "" );
	}
}

static void push_mru_entry( StrArray& list, const char* fullpath )
{
	for( unsigned int i=0; i<list.len(); )
	{
		if( 0 == ::lstrcmpi( (const char*)list[i], fullpath ) )
		{
			for( unsigned int j=i; j+1<list.len(); j++ )
				list[j] = list[j+1];
			list.forcelen( list.len()-1 );
			continue;
		}
		i++;
	}
	list.add( kiStr() );
	for( unsigned int i=list.len()-1; i>0; i-- )
		list[i] = list[i-1];
	list[0] = fullpath;
	if( list.len() > 10 )
		list.forcelen( 10 );
}

static void remove_mru_entry( StrArray& list, const char* fullpath )
{
	for( unsigned int i=0; i<list.len(); )
	{
		if( 0 == ::lstrcmpi( (const char*)list[i], fullpath ) )
		{
			for( unsigned int j=i; j+1<list.len(); j++ )
				list[j] = list[j+1];
			list.forcelen( list.len()-1 );
			continue;
		}
		i++;
	}
}

// Extract the last path component (without trailing separator) of a folder
// path that ends in '/' or '\' (e.g. "foo/bar/" -> "bar").
static void extract_folder_leaf( const char* path, char* out )
{
	int pathLen = ::lstrlen( path );
	int end = pathLen;
	if( end > 0 && (path[end-1] == '/' || path[end-1] == '\\') ) end--;
	int start = 0;
	for( int k = end - 1; k >= 0; k-- )
		if( path[k] == '/' || path[k] == '\\' ) { start = k + 1; break; }
	int n = end - start;
	for( int i = 0; i < n; i++ ) out[i] = path[start + i];
	out[n] = '\0';
}


BOOL CArcViewDlg::onInit()
{
	//-- Mark that one dialog has been created
	hello();

	//-- Capture ListView position BEFORE adding menu bar.
	//   SetMenu() triggers WM_SIZE; if m_listTop is still 0 at that point,
	//   LayoutPanes positions the ListView at y=0 and we capture 0 below.
	{
		RECT lr;
		::GetWindowRect( item(IDC_FILELIST), &lr );
		POINT pt = { lr.left, lr.top };
		::ScreenToClient( hwnd(), &pt );
		m_listLeft = pt.x;
		m_listTop  = pt.y;
		{ int s = mycnf().getInt("ArcViewSplit", 0); if(s > 0) m_listLeft = s; }
	}
	m_topRowH         = m_listTop;
	m_defaultListLeft = m_listLeft;

	//-- Menu bar
	m_hMenu = ::LoadMenu( app()->inst(), MAKEINTRESOURCE(IDR_ARCVIEW_MENU) );
	::SetMenu( hwnd(), m_hMenu );

	//-- Accelerator table
	loadAccel( IDR_ACCEL );

	//-- Accept external DnD (file dropped from Explorer into viewer)
	::DragAcceptFiles( hwnd(), TRUE );

	//-- Center and bring to front
	setCenter( hwnd(), app()->mainhwnd() );
	setFront( hwnd() );

	//-- Monospace font for raw archiver output
	{
		LOGFONT lf;
		::GetObject( (HFONT)sendMsg(WM_GETFONT), sizeof(lf), &lf );
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		lf.lfCharSet = DEFAULT_CHARSET;
		const char* fontName = mycnf().getStr( "ArcViewFont", "Consolas" );
		ki_strcpy( lf.lfFaceName, fontName );
		m_hFont = ::CreateFontIndirect( &lf );
	}
	::SendMessage( item(IDC_FILELIST), WM_SETFONT, (WPARAM)m_hFont, TRUE );
	::SendMessage( item(IDC_FILELIST), LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

	//-- System image list (needed for both ListView and TreeView)
	{
		SHFILEINFO sfi; ::ZeroMemory( &sfi, sizeof(sfi) );
		HIMAGELIST hImS = (HIMAGELIST)::SHGetFileInfo( "", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES );
		::SendMessage( item(IDC_FILELIST), LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImS );
		m_hTree = item( IDC_FOLDERTREE );
		::SendMessage( m_hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImS );
	}

	//-- Toolbar bitmap buttons
	{
		m_hBmpExtract  = (HBITMAP)::LoadImage( app()->inst(), MAKEINTRESOURCE(IDB_EXTRACT),
			IMAGE_BITMAP, 24, 24, LR_DEFAULTCOLOR );
		m_hBmpView     = (HBITMAP)::LoadImage( app()->inst(), MAKEINTRESOURCE(IDB_VIEW),
			IMAGE_BITMAP, 24, 24, LR_DEFAULTCOLOR );
		m_hBmpSettings = (HBITMAP)::LoadImage( app()->inst(), MAKEINTRESOURCE(IDB_SETTINGS),
			IMAGE_BITMAP, 24, 24, LR_DEFAULTCOLOR );
		HWND hMelt = item(IDC_MELTEACH), hShow = item(IDC_SHOW), hSett = item(IDC_SETTINGS);
		::SetWindowLong( hMelt, GWL_STYLE, ::GetWindowLong(hMelt,GWL_STYLE) | BS_BITMAP );
		::SetWindowLong( hShow, GWL_STYLE, ::GetWindowLong(hShow,GWL_STYLE) | BS_BITMAP );
		::SetWindowLong( hSett, GWL_STYLE, ::GetWindowLong(hSett,GWL_STYLE) | BS_BITMAP );
		::SendMessage( hMelt, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hBmpExtract );
		::SendMessage( hShow, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hBmpView );
		::SendMessage( hSett, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hBmpSettings );
	}

	//-- ListView header font
	m_hHeader = (HWND)sendMsgToItem( IDC_FILELIST, LVM_GETHEADER, 0, 0 );
	if( m_hHeader )
		::SendMessage( m_hHeader, WM_SETFONT, (WPARAM)m_hFont, TRUE );

	//-- Register DnD out format (always; giveData guards against empty state)
	{
		FORMATETC fmt;
		fmt.cfFormat = CF_HDROP;
		fmt.ptd      = NULL;
		fmt.dwAspect = DVASPECT_CONTENT;
		fmt.lindex   = -1;
		fmt.tymed    = TYMED_HGLOBAL;
		addFormat( fmt );
	}

	//-- Load MRU from INI
	load_mru_list( m_mruList );
	rebuildMRUMenu();

	//-- Restore saved window size
	{ int w = mycnf().getInt("ArcViewW", 0), h = mycnf().getInt("ArcViewH", 0);
	  if( w > 100 && h > 100 )
	      ::SetWindowPos(hwnd(), NULL, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); }

	//-- App icon (always)
	{ HICON _b=(HICON)::LoadImage(app()->inst(),MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,0,0,LR_SHARED|LR_DEFAULTSIZE);
	  HICON _s=(HICON)::LoadImage(app()->inst(),MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,::GetSystemMetrics(SM_CXSMICON),::GetSystemMetrics(SM_CYSMICON),LR_SHARED);
	  sendMsg(WM_SETICON,ICON_BIG,(LPARAM)_b); sendMsg(WM_SETICON,ICON_SMALL,(LPARAM)_s); }

	//-- Load archive content (or show empty state)
	if( m_pArc != NULL )
	{
		//-- Title: "Noah - filename"
		{ char _t[MAX_PATH+10]; wsprintf(_t, "Noah - %s", kiPath((const char*)m_arcLongName).name()); sendMsg(WM_SETTEXT,0,(LPARAM)_t); }
		updateDdirDisplay();
		rebuildContent();
	}
	else
	{
		//-- Empty state: no archive loaded
		sendMsg( WM_SETTEXT, 0, (LPARAM)"Noah" );
		setOperationControls( false );
		kiListView ctrl( this, IDC_FILELIST );
		ctrl.insertColumn( 0, "", 510 );
	}

	//-- Layout
	{
		RECT rc; ::GetClientRect( hwnd(), &rc );
		RECT sbar; ::GetClientRect( item(IDC_STATUSBAR), &sbar );
		int sbH = sbar.bottom - sbar.top;
		int paneH = rc.bottom - m_listTop - sbH;
		LayoutTopRow( rc.right );
		LayoutPanes( rc.right, paneH );
		::SetWindowPos( item(IDC_STATUSBAR), NULL, 0,
			rc.bottom - sbH, 0, 0, SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	}

	return FALSE;
}

bool CArcViewDlg::onOK()
{
	// IsDialogMessage converts Enter to WM_COMMAND(IDOK) when a non-button
	// control has focus. Treat it as IDC_SHOW when the list has focus;
	// never close the main window on Enter.
	if( ::GetFocus() == item(IDC_FILELIST) )
		sendMsg( WM_COMMAND, IDC_SHOW );
	return false;
}

bool CArcViewDlg::onCancel()
{
	//-- Save window size and splitter position
	{ RECT wr; ::GetWindowRect(hwnd(), &wr);
	  mycnf().putInt("ArcViewW",     wr.right  - wr.left);
	  mycnf().putInt("ArcViewH",     wr.bottom - wr.top);
	  mycnf().putInt("ArcViewSplit", m_listLeft); }

	if( m_pArc )
		::SetCurrentDirectory( m_arcBaseDir );
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
	if( m_hFont        ) { ::DeleteObject(m_hFont);        m_hFont        = NULL; }
	if( m_hBmpExtract  ) { ::DeleteObject(m_hBmpExtract);  m_hBmpExtract  = NULL; }
	if( m_hBmpView     ) { ::DeleteObject(m_hBmpView);     m_hBmpView     = NULL; }
	if( m_hBmpSettings ) { ::DeleteObject(m_hBmpSettings); m_hBmpSettings = NULL; }
	byebye();
	return true;
}

bool CArcViewDlg::giveData( const FORMATETC& fmt, STGMEDIUM* stg, bool firstcall )
{
	if( !m_pArc ) return false;

	if( firstcall )
		if( 0x8000<=m_pArc->melt( makeArcName(), m_tdir, &m_files ) )
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
			kiStr::replaceToBackslash( buf );
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
	//-- Re-assert wait cursor each message pump cycle while loading -----------
	case WM_TIMER:
		if( m_bLoading )
		{
			::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
			return TRUE;
		}
		break;

	//-- Specify main window ---------------------
	case WM_ACTIVATE:
		if( LOWORD(wp)==WA_ACTIVE || LOWORD(wp)==WA_CLICKACTIVE )
		{
			app()->setMainWnd( this );
			return TRUE;
		}
		break;

	//-- External file dropped onto viewer ---------------------
	case WM_DROPFILES:
		{
			char buf[MAX_PATH];
			HDROP drop = (HDROP)wp;
			::DragQueryFile( drop, 0, buf, MAX_PATH );
			::DragFinish( drop );
			handleDroppedFile( buf );
			return TRUE;
		}

	//-- Dynamic menu enable/check states ---------------------
	case WM_INITMENUPOPUP:
		{
			HMENU hSub = (HMENU)wp;
			bool hasArc = (m_pArc != NULL);
			bool hasSel = hasArc && (::SendMessage(item(IDC_FILELIST), LVM_GETSELECTEDCOUNT, 0, 0) > 0);
			::EnableMenuItem(hSub, IDM_FILE_CLOSE,
				MF_BYCOMMAND|(hasArc ? MF_ENABLED : MF_GRAYED));
			::EnableMenuItem(hSub, IDM_OPS_EXTRACTALL,
				MF_BYCOMMAND|(hasArc&&m_bAble ? MF_ENABLED : MF_GRAYED));
			::EnableMenuItem(hSub, IDM_OPS_EXTRACTSEL,
				MF_BYCOMMAND|(hasSel&&m_bAble ? MF_ENABLED : MF_GRAYED));
			::EnableMenuItem(hSub, IDM_OPS_OPEN,
				MF_BYCOMMAND|(hasSel&&m_bAble ? MF_ENABLED : MF_GRAYED));
			::CheckMenuItem(hSub, IDM_VIEW_TOOLBAR,
				MF_BYCOMMAND|(m_bShowToolbar ? MF_CHECKED : MF_UNCHECKED));
			::CheckMenuItem(hSub, IDM_VIEW_TREE,
				MF_BYCOMMAND|(m_bShowTree    ? MF_CHECKED : MF_UNCHECKED));
			return FALSE;
		}

	//-- Resize-related processing ---------------------
	case WM_GETMINMAXINFO:
		{
			RECT self, client;
			::GetWindowRect( hwnd(), &self );
			::GetClientRect( hwnd(), &client );
			int frameW = (self.right-self.left) - (client.right-client.left);
			POINT& sz = ((MINMAXINFO*)lp)->ptMinTrackSize;
			RECT ref; ::GetWindowRect( item(IDC_REF), &ref );
			if( m_bShowToolbar )
			{
				RECT label,melt,show,sett;
				::GetWindowRect( item(IDC_DDIRLABEL), &label );
				::GetWindowRect( item(IDC_MELTEACH),  &melt );
				::GetWindowRect( item(IDC_SHOW),      &show );
				::GetWindowRect( item(IDC_SETTINGS),  &sett );
				const int minEdit = 100;
				const int margin = 4, gap = 2, groupGap = 10, labelGap = -8;
				int minClientW = margin
					+ (melt.right-melt.left) + gap
					+ (show.right-show.left) + gap
					+ (sett.right-sett.left) + groupGap
					+ (label.right-label.left) + labelGap
					+ minEdit + gap + (ref.right-ref.left) + margin;
				sz.x = minClientW + frameW;
			}
			else
			{
				sz.x = 200 + frameW;
			}
			sz.y = ref.bottom - self.top + 100;
		}
		return TRUE;

	case WM_SIZE:
		if( wp!=SIZE_MAXHIDE && wp!=SIZE_MINIMIZED )
		{
			int dlgW = (int)(short)LOWORD(lp);
			int dlgH = (int)(short)HIWORD(lp);
			RECT sbar; ::GetClientRect( item(IDC_STATUSBAR), &sbar );
			int sbH = sbar.bottom - sbar.top;
			int paneH = dlgH - m_listTop - sbH;
			if( m_bShowToolbar ) LayoutTopRow( dlgW );
			LayoutPanes( dlgW, paneH );
			::SetWindowPos( item(IDC_STATUSBAR), NULL, 0,
				dlgH - sbH, 0, 0, SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER );
		}
		break;

	case WM_SETCURSOR:
		if( m_bLoading )
		{
			::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
			return TRUE;
		}
		if( LOWORD(lp)==HTCLIENT )
		{
			POINT pt; ::GetCursorPos(&pt); ::ScreenToClient(hwnd(),&pt);
			if( pt.x >= m_listLeft-ARCVIEW_SPLITTER_WIDTH && pt.x < m_listLeft )
			{
				::SetCursor( ::LoadCursor(NULL, IDC_SIZEWE) );
				return TRUE;
			}
		}
		break;

	case WM_LBUTTONDOWN:
		{
			int x = (int)(short)LOWORD(lp);
			if( x >= m_listLeft-ARCVIEW_SPLITTER_WIDTH && x < m_listLeft )
			{
				m_bDragging    = true;
				m_dragX        = x;
				m_dragListLeft = m_listLeft;
				::SetCapture( hwnd() );
				::SetCursor( ::LoadCursor(NULL, IDC_SIZEWE) );
				return TRUE;
			}
		}
		break;

	case WM_MOUSEMOVE:
		if( m_bDragging )
		{
			DrawSplitterGhost( m_ghostX );  // erase old
			int newLeft = m_dragListLeft + (int)(short)LOWORD(lp) - m_dragX;
			RECT rc; ::GetClientRect( hwnd(), &rc );
			const int SP=ARCVIEW_SPLITTER_WIDTH, minTree=30, minList=80;
			if( newLeft < minTree+SP )      newLeft = minTree+SP;
			if( newLeft > rc.right-minList ) newLeft = rc.right-minList;
			m_listLeft = newLeft;
			m_ghostX   = newLeft - SP/2;  // center of splitter strip
			DrawSplitterGhost( m_ghostX );  // draw at new position
		}
		break;

	case WM_LBUTTONUP:
		if( m_bDragging )
		{
			DrawSplitterGhost( m_ghostX );  // erase ghost
			m_ghostX    = -1;
			m_bDragging = false;
			::ReleaseCapture();
			RECT rc, lr;
			::GetClientRect( hwnd(), &rc );
			::GetWindowRect( item(IDC_FILELIST), &lr );
			int dlgW  = rc.right;
			int paneH = lr.bottom - lr.top;
			LayoutPanes( dlgW, paneH );
			::RedrawWindow( hwnd(), NULL, NULL,
				RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN );
			// Save as new default split for tree-toggle restore
			m_defaultListLeft = m_listLeft;
		}
		break;

	case WM_CAPTURECHANGED:
		if( m_bDragging )
		{
			DrawSplitterGhost( m_ghostX );
			m_ghostX    = -1;
			m_bDragging = false;
		}
		break;

	case WM_NOTIFY:
		if( wp==IDC_FOLDERTREE )
		{
			NMTREEVIEW* ptv = (NMTREEVIEW*)lp;
			if( ptv->hdr.code == TVN_SELCHANGED )
			{
				FilterListByFolder( (int)ptv->itemNew.lParam );
				return TRUE;
			}
		}
		if( wp==IDC_FILELIST && m_bAble )
		{
			NMHDR* phdr=(NMHDR*)lp;
			if( phdr->code==LVN_COLUMNCLICK )
			{
				NMLISTVIEW* plv = (NMLISTVIEW*)lp;
				if( plv->iSubItem == 0 )
				{
					m_sortDir = (m_sortDir == 1) ? -1 : 1;
					FilterListByFolder( m_curFolderIdx );
				}
				return TRUE;
			}
			else if( phdr->code==LVN_BEGINDRAG || phdr->code==LVN_BEGINRDRAG )
			{
				clearSelections();
				if( setSelection() )
					kiDropSource::DnD( this, DROPEFFECT_COPY );
				return TRUE;
			}
			else if( phdr->code==NM_DBLCLK )
			{
				NMITEMACTIVATE* pia = (NMITEMACTIVATE*)lp;
				if( pia->iItem >= 0 )
				{
					LVITEM it;
					it.mask = LVIF_PARAM;
					it.iItem = pia->iItem;
					it.iSubItem = 0;
					if( sendMsgToItem( IDC_FILELIST, LVM_GETITEM, 0, (LPARAM)&it ) )
					{
						listrow* r = (listrow*)it.lParam;
						if( r && r->isFolder )
						{
							HTREEITEM hTarget = NULL;
							if( r->idx < 0 )
								hTarget = (HTREEITEM)::SendMessage( m_hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0 );
							else if( (unsigned int)r->idx < m_treeNodes.len() )
								hTarget = m_treeNodes[r->idx];
							if( hTarget )
								::SendMessage( m_hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hTarget );
							return TRUE;
						}
					}
				}
				sendMsg( WM_COMMAND, IDC_SHOW );
			}
			else if( phdr->code==NM_RETURN )
			{
				sendMsg( WM_COMMAND, IDC_SHOW );
				return TRUE;
			}
			else if( phdr->code==NM_RCLICK )
			{
				POINT pt; ::GetCursorPos( &pt );
				clearSelections();
				if( setSelection() )
					DoRMenu( pt );
			}
		}
		break;

	case WM_CONTEXTMENU:
		// Application key (lParam==-1) while ListView has focus: show context menu
		// at the selected item's position. Mouse right-click is handled via NM_RCLICK.
		if( (HWND)wp == item(IDC_FILELIST) && lp == (LPARAM)-1 && m_pArc && m_bAble )
		{
			POINT pt = {0, 0};
			int iSel = (int)::SendMessage( item(IDC_FILELIST), LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED );
			if( iSel < 0 )
				iSel = (int)::SendMessage( item(IDC_FILELIST), LVM_GETSELECTIONMARK, 0, 0 );
			if( iSel >= 0 )
			{
				RECT rc; rc.left = LVIR_BOUNDS;
				if( ::SendMessage( item(IDC_FILELIST), LVM_GETITEMRECT, iSel, (LPARAM)&rc ) )
					pt.x = rc.left, pt.y = rc.bottom;
			}
			::ClientToScreen( item(IDC_FILELIST), &pt );
			clearSelections();
			if( setSelection() )
				DoRMenu( pt );
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch( LOWORD(wp) )
		{
		case IDC_SETTINGS:
			mycnf().dialog();
			return TRUE;

		case IDC_REF: // Set extraction destination
			kiSUtil::getFolderDlgOfEditBox( item(IDC_DDIR), hwnd(), kiStr().loadRsrc(IDS_CHOOSEDIR) );
			return TRUE;

		case IDC_MELTEACH: // Extract selected entries, or all when nothing is selected
			if( m_pArc && m_bAble )
			{
				clearSelections();
				setdir();
				int result = setSelection()
					? m_pArc->melt( makeArcName(), m_ddir, &m_files )
					: m_pArc->melt( makeArcName(), m_ddir );
				reportMeltResult( result );
			}
			return TRUE;

		case IDC_SHOW: // Show
			if( m_pArc && m_bAble )
			{
				// If a folder row is selected, navigate into it instead of viewing.
				LVITEM it;
				it.mask = LVIF_PARAM | LVIF_STATE;
				it.iSubItem = 0;
				it.stateMask = LVIS_SELECTED;
				bool folderFound = false;
				int folderTreeIdx = -1;
				for( it.iItem = 0;
				     sendMsgToItem( IDC_FILELIST, LVM_GETITEM, 0, (LPARAM)&it );
				     it.iItem++ )
				{
					if( 0 == (LVIS_SELECTED & it.state) ) continue;
					listrow* r = (listrow*)it.lParam;
					if( r && r->isFolder )
					{ folderFound = true; folderTreeIdx = r->idx; break; }
				}
				if( folderFound )
				{
					HTREEITEM hTarget = NULL;
					if( folderTreeIdx < 0 )
						hTarget = (HTREEITEM)::SendMessage( m_hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0 );
					else if( (unsigned int)folderTreeIdx < m_treeNodes.len() )
						hTarget = m_treeNodes[folderTreeIdx];
					if( hTarget )
						::SendMessage( m_hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hTarget );
					return TRUE;
				}
				clearSelections();
				if( setSelection() )
					openSelected();
			}
			return TRUE;

		//-- Menu: File
		case IDM_FILE_OPEN:
			doFileOpen();
			return TRUE;

		case IDM_FILE_CLOSE:
			unloadArchive();
			return TRUE;

		case IDM_FILE_EXIT:
			onCancel();
			return TRUE;

		//-- Accelerator: My Dir
		case IDA_MYDIR:
			myapp().open_folder( kiPath( kiPath::Exe ) );
			return TRUE;

		//-- Accelerator: Help
		case IDA_HELP:
			mycnf().dialog( 2 );  // Help tab
			return TRUE;

		//-- Menu: Operations
		case IDM_OPS_EXTRACTALL:
			if( m_pArc && m_bAble )
			{
				setdir();
				int result = m_pArc->melt( makeArcName(), m_ddir );
				reportMeltResult( result );
			}
			return TRUE;

		case IDM_OPS_EXTRACTSEL:
			if( m_pArc && m_bAble )
			{
				clearSelections();
				if( setSelection() )
				{
					setdir();
					int result = m_pArc->melt( makeArcName(), m_ddir, &m_files );
					reportMeltResult( result );
				}
			}
			return TRUE;

		case IDM_OPS_OPEN:
			if( m_pArc && m_bAble )
			{
				clearSelections();
				if( setSelection() )
					openSelected();
			}
			return TRUE;

		case IDM_OPS_SETTINGS:
			mycnf().dialog();
			return TRUE;

		//-- Menu: View
		case IDM_VIEW_TOOLBAR:
			m_bShowToolbar = !m_bShowToolbar;
			applyToolbarVisibility();
			return TRUE;

		case IDM_VIEW_TREE:
			m_bShowTree = !m_bShowTree;
			applyTreeVisibility();
			return TRUE;

		//-- Menu: Help
		case IDM_HELP_ABOUT:
			mycnf().dialog( 2 );
			return TRUE;

		default:
			if( LOWORD(wp) >= IDM_FILE_MRU_FIRST && LOWORD(wp) <= IDM_FILE_MRU_LAST )
			{
				unsigned int idx = LOWORD(wp) - IDM_FILE_MRU_FIRST;
				if( idx < m_mruList.len() )
				{
					char local[MAX_PATH];
					::lstrcpy( local, (const char*)m_mruList[idx] );
					if( !kiSUtil::exist(local) )
					{
						char msg[MAX_PATH + 64];
						wsprintf( msg, "File not found:\n%s\n\nThis entry will be removed from history.", local );
						app()->msgBox( msg, "Noah" );
						load_mru_list( m_mruList );
						remove_mru_entry( m_mruList, local );
						persist_mru_list( m_mruList );
						rebuildMRUMenu();
					}
					else
						openFromPath( local );
				}
				return TRUE;
			}
			break;
		}
		break;
	}
	return FALSE;
}

bool CArcViewDlg::setSelection()
{
	bool x = false;
	LVITEM it;
	it.mask = LVIF_PARAM | LVIF_STATE;
	it.iSubItem = 0;
	it.stateMask = LVIS_SELECTED;
	for( it.iItem = 0;
	     sendMsgToItem( IDC_FILELIST, LVM_GETITEM, 0, (LPARAM)&it );
	     it.iItem++ )
	{
		if( 0 == (LVIS_SELECTED & it.state) ) continue;
		listrow* r = (listrow*)it.lParam;
		if( !r ) continue;
		if( r->isFolder )
		{
			// Recursively select every file under this folder path.
			if( r->idx < 0 || (unsigned int)r->idx >= m_folderPaths.len() ) continue;
			const char* pfx = m_folderPaths[ r->idx ];
			int pfxLen = ::lstrlen( pfx );
			for( unsigned int fi = 0; fi < m_fileIndices.len(); fi++ )
			{
				arcfile& af = m_files[ m_fileIndices[fi] ];
				if( (int)::lstrlen(af.inf.szFileName) >= pfxLen
				 && ki_memcmp(af.inf.szFileName, pfx, pfxLen) )
				{
					af.selected = true;
					x = true;
				}
			}
		}
		else
		{
			if( r->idx < 0 || (unsigned int)r->idx >= m_fileIndices.len() ) continue;
			m_files[ m_fileIndices[ r->idx ] ].selected = true;
			x = true;
		}
	}
	return x;
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

// Extract the entries currently flagged in m_files to the temp directory and
// open each with its associated application. The caller is responsible for
// setting the m_files[].selected flags beforehand.
void CArcViewDlg::openSelected()
{
	int assocCnt = hlp_cnt_check();
	if( 0x8000 > m_pArc->melt( makeArcName(), m_tdir, &m_files ) )
	{
		if( assocCnt != -1 )
			m_files[assocCnt].selected = false;
		for( unsigned i=0; i!=m_files.len(); i++ )
			if( m_files[i].selected )
			{
				kiPath tmp(m_tdir);
				char yen[MAX_PATH];
				ki_strcpy( yen, m_files[i].inf.szFileName );
				kiStr::replaceToBackslash( yen );
				tmp += yen;
				::ShellExecute( hwnd(), NULL, tmp, NULL, m_tdir, SW_SHOWDEFAULT );
			}
	}
	kiSUtil::switchCurDirToExeDir(); // Just to be safe
}

void CArcViewDlg::updateDdirDisplay()
{
	char longdir[MAX_PATH];
	if( ::GetLongPathName((const char*)m_ddir, longdir, MAX_PATH) == 0 )
	{
		// GetLongPathName fails when the path ends in a not-yet-existing
		// component (e.g. a subdirectory generated by generate_dirname).
		// Strip trailing backslash first so kiPath::name() returns the leaf,
		// then convert the parent directory and re-append the leaf.
		char notrail[MAX_PATH];
		::lstrcpy( notrail, (const char*)m_ddir );
		int n = ::lstrlen(notrail);
		if( n > 0 && (notrail[n-1] == '\\' || notrail[n-1] == '/') )
			notrail[n-1] = '\0';
		const char* leafPtr = kiPath::name( notrail );
		char leaf[MAX_PATH];
		::lstrcpy( leaf, leafPtr );
		kiPath parent( notrail );
		if( parent.beDirOnly() &&
			::GetLongPathName((const char*)parent, longdir, MAX_PATH) > 0 )
		{
			int len = ::lstrlen(longdir);
			if( len > 0 && longdir[len-1] != '\\' )
				longdir[len++] = '\\', longdir[len] = '\0';
			::lstrcat( longdir, leaf );
		}
		else
		{
			::lstrcpy( longdir, (const char*)m_ddir );
		}
	}
	sendMsgToItem( IDC_DDIR, WM_SETTEXT, 0, (LPARAM)longdir );
}

void CArcViewDlg::setOperationControls( bool enable )
{
	static const UINT items[] = { IDC_REF,IDC_MELTEACH,IDC_SHOW,IDC_DDIR,IDC_DDIRLABEL };
	BOOL en = enable ? TRUE : FALSE;
	for( int i=0; i<5; i++ )
		::EnableWindow( item(items[i]), en );
}

void CArcViewDlg::reportMeltResult( int result )
{
	if( result < 0x8000 )
		myapp().open_folder( m_ddir, 1 );
	else if( result != 0x8020 )
	{
		char str[255];
		wsprintf( str, "%s\nError No: [%x]",
			(const char*)kiStr().loadRsrc( IDS_M_ERROR ), result );
		app()->msgBox( str );
	}
	kiSUtil::switchCurDirToExeDir();
}

void CArcViewDlg::LayoutTopRow( int dlgW )
{
	const int margin   = 4;
	const int gap      = 2;
	const int labelGap = -8;
	const int sepW     = 2;
	const int sepGap   = 4;

	RECT rcLabel, rcEdit, rcRef, rcMelt, rcShow, rcSett;
	::GetWindowRect( item(IDC_DDIRLABEL), &rcLabel );
	::GetWindowRect( item(IDC_DDIR),      &rcEdit );
	::GetWindowRect( item(IDC_REF),       &rcRef );
	::GetWindowRect( item(IDC_MELTEACH),  &rcMelt );
	::GetWindowRect( item(IDC_SHOW),      &rcShow );
	::GetWindowRect( item(IDC_SETTINGS),  &rcSett );

	const int labelW = rcLabel.right - rcLabel.left;
	const int labelH = rcLabel.bottom - rcLabel.top;
	const int editH  = rcEdit.bottom - rcEdit.top;
	const int refW   = rcRef.right - rcRef.left;
	const int refH   = rcRef.bottom - rcRef.top;
	const int meltW  = rcMelt.right - rcMelt.left;
	const int meltH  = rcMelt.bottom - rcMelt.top;
	const int showW  = rcShow.right - rcShow.left;
	const int showH  = rcShow.bottom - rcShow.top;
	const int settW  = rcSett.right - rcSett.left;
	const int settH  = rcSett.bottom - rcSett.top;
	int topBandH = m_listTop;
	if( topBandH <= 0 ) topBandH = 20;
	const int shiftY = 2;
	const int meltY  = (topBandH - meltH ) / 2 + shiftY;
	const int showY  = (topBandH - showH ) / 2 + shiftY;
	const int settY  = (topBandH - settH ) / 2 + shiftY;
	const int labelY = (topBandH - labelH) / 2 + shiftY;
	const int editY  = (topBandH - editH ) / 2 + shiftY;
	const int refY   = (topBandH - refH  ) / 2 + shiftY;
	const int sepH   = meltH - 4;
	const int sepY   = (topBandH - sepH) / 2 + shiftY;

	// Icon buttons on the LEFT, separators around Settings, label+edit+browse on the right
	int x = margin;
	const int meltX  = x; x += meltW + gap;
	const int showX  = x; x += showW + sepGap;
	const int sep1X  = x; x += sepW + sepGap;
	const int settX  = x; x += settW + sepGap;
	const int sep2X  = x; x += sepW + sepGap;
	const int labelX = x; x += labelW + labelGap;
	const int editX  = x;
	const int refX   = dlgW - margin - refW;
	const int editW  = refX - gap - editX;

	HDWP hdwp = ::BeginDeferWindowPos( 8 );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_MELTEACH), NULL,
			meltX, meltY, meltW, meltH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_SHOW), NULL,
			showX, showY, showW, showH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_SEP1), NULL,
			sep1X, sepY, sepW, sepH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_SETTINGS), NULL,
			settX, settY, settW, settH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_SEP2), NULL,
			sep2X, sepY, sepW, sepH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_DDIRLABEL), NULL,
			labelX, labelY, labelW, labelH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_DDIR), NULL,
			editX, editY, editW, editH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_REF), NULL,
			refX, refY, refW, refH, SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		::EndDeferWindowPos( hdwp );
}

void CArcViewDlg::DrawSplitterGhost( int x )
{
	if( x < 0 ) return;
	RECT cr; ::GetClientRect( hwnd(), &cr );
	RECT sr; ::GetClientRect( item(IDC_STATUSBAR), &sr );
	int lineH = cr.bottom - m_listTop - (sr.bottom - sr.top);
	HDC hdc = ::GetDC( hwnd() );
	HBRUSH old = (HBRUSH)::SelectObject( hdc, ::GetStockObject(GRAY_BRUSH) );
	::PatBlt( hdc, x, m_listTop, 1, lineH, PATINVERT );
	::SelectObject( hdc, old );
	::ReleaseDC( hwnd(), hdc );
}

void CArcViewDlg::LayoutPanes( int dlgW, int paneH )
{
	const int SP = ARCVIEW_SPLITTER_WIDTH; // splitter strip width (pixels, dialog client area)
	const int minTree = 30;
	const int minList = 80;

	if( !m_bShowTree )
	{
		// Tree hidden: ListView fills the full width
		HDWP hdwp = ::BeginDeferWindowPos(1);
		if( hdwp )
			hdwp = ::DeferWindowPos( hdwp, item(IDC_FILELIST), NULL,
				0, m_listTop, dlgW, paneH, SWP_NOOWNERZORDER|SWP_NOZORDER );
		if( hdwp ) ::EndDeferWindowPos( hdwp );
		return;
	}

	if( m_listLeft < minTree + SP ) m_listLeft = minTree + SP;
	if( m_listLeft > dlgW - minList ) m_listLeft = dlgW - minList;

	// Update both controls atomically to avoid flicker
	HDWP hdwp = ::BeginDeferWindowPos(2);
	if( hdwp && m_hTree )
		hdwp = ::DeferWindowPos( hdwp, m_hTree, NULL,
			0, m_listTop, m_listLeft - SP, paneH,
			SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		hdwp = ::DeferWindowPos( hdwp, item(IDC_FILELIST), NULL,
			m_listLeft, m_listTop, dlgW - m_listLeft, paneH,
			SWP_NOOWNERZORDER|SWP_NOZORDER );
	if( hdwp )
		::EndDeferWindowPos( hdwp );

	// Repaint splitter strip only
	RECT sr = { m_listLeft - SP, m_listTop, m_listLeft, m_listTop + paneH };
	::InvalidateRect( hwnd(), &sr, TRUE );
}

// Binary search in m_folderSorted for the path buf.
// Returns the index in m_folderSorted where buf is (or should be inserted).
// Sets *found = true if an exact match exists.
static unsigned int folder_bisect( const StrArray& paths, const kiArray<unsigned int>& sorted,
	const char* buf, bool* found )
{
	unsigned int lo = 0, hi = sorted.len();
	while( lo < hi )
	{
		unsigned int mid = (lo + hi) / 2;
		int cmp = ::lstrcmp( paths[ sorted[mid] ], buf );
		if( cmp == 0 ) { *found = true; return mid; }
		if( cmp < 0 )  lo = mid + 1;
		else           hi = mid;
	}
	*found = false;
	return lo;
}

void CArcViewDlg::BuildFolderTree( HTREEITEM hRoot, int folderIconIdx )
{
	for( unsigned int fi=0; fi!=m_fileIndices.len(); fi++ )
	{
		const char* fn = m_files[ m_fileIndices[fi] ].inf.szFileName;
		for( const char* p=fn; *p; p++ )
		{
			if( *p != '/' && *p != '\\' ) continue;

			int len = (int)(p - fn) + 1;
			char buf[MAX_PATH];
			if( len >= MAX_PATH ) continue;
			::CopyMemory( buf, fn, len );
			buf[len] = '\0';

			// Skip if already registered (binary search)
			bool found;
			unsigned int pos = folder_bisect( m_folderPaths, m_folderSorted, buf, &found );
			if( found ) continue;

			// Find parent node (strip last component from buf, then binary search)
			HTREEITEM hParent = hRoot;
			for( int k=len-2; k>=0; k-- )
			{
				if( buf[k] == '/' || buf[k] == '\\' )
				{
					char pbuf[MAX_PATH];
					::CopyMemory( pbuf, buf, k+1 );
					pbuf[k+1] = '\0';
					bool pfound;
					unsigned int ppos = folder_bisect( m_folderPaths, m_folderSorted, pbuf, &pfound );
					if( pfound )
						hParent = m_treeNodes[ m_folderSorted[ppos] ];
					break;
				}
			}

			// Node label: last path component (excluding trailing separator)
			int labelStart = 0;
			for( int k=len-2; k>=0; k-- )
				if( buf[k] == '/' || buf[k] == '\\' ) { labelStart = k+1; break; }
			char label[MAX_PATH];
			int labelLen = len - 1 - labelStart;
			::CopyMemory( label, buf+labelStart, labelLen );
			label[labelLen] = '\0';

			TVINSERTSTRUCT tvis;
			::ZeroMemory( &tvis, sizeof(tvis) );
			tvis.hParent      = hParent;
			tvis.hInsertAfter = TVI_SORT;
			tvis.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvis.item.pszText = label;
			tvis.item.lParam  = (LPARAM)m_folderPaths.len();
			tvis.item.iImage         = folderIconIdx;
			tvis.item.iSelectedImage = folderIconIdx;
			HTREEITEM h = (HTREEITEM)::SendMessage( m_hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis );

			// Register: append to paths/nodes, insert index into sorted list
			unsigned int newIdx = m_folderPaths.len();
			m_folderPaths.add( kiStr(buf) );
			m_treeNodes.add( h );
			m_folderSorted.add( 0 );  // extend by 1 (value overwritten below)
			for( unsigned int k=newIdx; k>pos; k-- )
				m_folderSorted[k] = m_folderSorted[k-1];
			m_folderSorted[pos] = newIdx;
		}
	}
}

void CArcViewDlg::FilterListByFolder( int folderIdx )
{
	m_curFolderIdx = folderIdx;

	// Clear selection on all file entries before rebuilding the list
	clearSelections();

	kiListView ctrl( this, IDC_FILELIST );
	::SendMessage( item(IDC_FILELIST), WM_SETREDRAW, FALSE, 0 );
	::SendMessage( item(IDC_FILELIST), LVM_DELETEALLITEMS, 0, 0 );
	m_rows.empty();

	const char* prefix    = NULL;
	int         prefixLen = 0;
	if( folderIdx >= 0 && (unsigned int)folderIdx < m_folderPaths.len() )
	{
		prefix    = m_folderPaths[folderIdx];
		prefixLen = ::lstrlen( prefix );
	}

	// Collect immediate child folders of the current prefix. A path in
	// m_folderPaths ends in a separator, so a direct child has exactly one
	// separator (at the end) after stripping the prefix.
	kiArray<unsigned int> childFolders;
	for( unsigned int j=0; j<m_folderPaths.len(); j++ )
	{
		const char* path = m_folderPaths[j];
		int pathLen = ::lstrlen( path );
		const char* tail; int tailLen;
		if( prefix )
		{
			if( pathLen <= prefixLen ) continue;
			if( !ki_memcmp(path, prefix, prefixLen) ) continue;
			tail = path + prefixLen;
			tailLen = pathLen - prefixLen;
		}
		else
		{
			tail = path;
			tailLen = pathLen;
		}
		int sepCount = 0, lastSepPos = -1;
		for( int k=0; k<tailLen; k++ )
			if( tail[k]=='/' || tail[k]=='\\' )
			{ sepCount++; lastSepPos = k; }
		if( sepCount != 1 || lastSepPos != tailLen - 1 ) continue;
		childFolders.add( j );
	}

	// Sort child folders by leaf name (case-insensitive).
	for( unsigned int a=1; a<childFolders.len(); a++ )
	{
		unsigned int key = childFolders[a];
		char keyLeaf[MAX_PATH];
		extract_folder_leaf( m_folderPaths[key], keyLeaf );
		int b = (int)a - 1;
		while( b >= 0 )
		{
			char otherLeaf[MAX_PATH];
			extract_folder_leaf( m_folderPaths[childFolders[b]], otherLeaf );
			if( ::lstrcmpi(otherLeaf, keyLeaf) <= 0 ) break;
			childFolders[b+1] = childFolders[b];
			b--;
		}
		childFolders[b+1] = key;
	}

	unsigned int k = 0;

	// Insert ".." entry when browsing a subfolder.
	if( prefix )
	{
		int parentIdx = -1;
		{
			int end = prefixLen - 1;
			while( end > 0 && prefix[end-1] != '/' && prefix[end-1] != '\\' )
				end--;
			if( end > 0 && end < MAX_PATH )
			{
				char pbuf[MAX_PATH];
				::CopyMemory( pbuf, prefix, end );
				pbuf[end] = '\0';
				bool pfound;
				unsigned int ppos = folder_bisect( m_folderPaths, m_folderSorted, pbuf, &pfound );
				if( pfound )
					parentIdx = (int)m_folderSorted[ppos];
			}
		}
		listrow lr; lr.isFolder = true; lr.idx = parentIdx;
		m_rows.add( lr );
		ctrl.insertItem( k, "..", (LPARAM)&m_rows[m_rows.len()-1], m_folderIconIdx );
		k++;
	}

	// Insert folder rows first.
	for( unsigned int a=0; a<childFolders.len(); a++ )
	{
		unsigned int fp = childFolders[a];
		char leaf[MAX_PATH];
		extract_folder_leaf( m_folderPaths[fp], leaf );
		listrow lr; lr.isFolder = true; lr.idx = (int)fp;
		m_rows.add( lr );
		ctrl.insertItem( k, leaf, (LPARAM)&m_rows[m_rows.len()-1], m_folderIconIdx );
		if( fp < m_folderRawlines.len() && ((const char*)m_folderRawlines[fp])[0] )
			ctrl.setSubItem( k, 1, (const char*)m_folderRawlines[fp] );
		k++;
	}

	// Collect files directly in the selected folder.
	kiArray<unsigned int> matchFiles;
	for( unsigned int fi=0; fi<m_fileIndices.len(); fi++ )
	{
		const char* fn = m_files[ m_fileIndices[fi] ].inf.szFileName;
		const char* lastSep = NULL;
		for( const char* p=fn; *p; p++ )
			if( *p=='/' || *p=='\\' ) lastSep = p;
		int dirLen = lastSep ? (int)(lastSep - fn) + 1 : 0;
		if( prefix )
		{
			if( dirLen != prefixLen || !ki_memcmp(fn, prefix, prefixLen) )
				continue;
		}
		else
		{
			if( dirLen != 0 )
				continue;
		}
		matchFiles.add( fi );
	}

	// Sort by name (insertion sort).
	if( m_sortDir != 0 )
	{
		for( unsigned int a=1; a<matchFiles.len(); a++ )
		{
			unsigned int key = matchFiles[a];
			const char* keyName = kiPath::name( m_files[m_fileIndices[key]].inf.szFileName );
			int b = (int)a - 1;
			while( b >= 0 )
			{
				const char* oName = kiPath::name( m_files[m_fileIndices[matchFiles[b]]].inf.szFileName );
				int cmp = ::lstrcmpi( oName, keyName );
				if( m_sortDir > 0 ? cmp <= 0 : cmp >= 0 ) break;
				matchFiles[b+1] = matchFiles[b];
				b--;
			}
			matchFiles[b+1] = key;
		}
	}

	// Insert files.
	for( unsigned int a=0; a<matchFiles.len(); a++ )
	{
		unsigned int fi = matchFiles[a];
		arcfile& af = m_files[ m_fileIndices[fi] ];
		listrow lr; lr.isFolder = false; lr.idx = (int)fi;
		m_rows.add( lr );
		ctrl.insertItem( k, kiPath::name(af.inf.szFileName),
			(LPARAM)&m_rows[m_rows.len()-1], cachedIconFor(af.inf.szFileName) );
		ctrl.setSubItem( k, 1, af.rawline );
		k++;
	}

	// Update sort arrow on Name column header.
	if( m_hHeader )
	{
		HDITEM hdi; ::ZeroMemory( &hdi, sizeof(hdi) );
		hdi.mask = HDI_FORMAT;
		::SendMessage( m_hHeader, HDM_GETITEM, 0, (LPARAM)&hdi );
		hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if( m_sortDir > 0 )      hdi.fmt |= HDF_SORTUP;
		else if( m_sortDir < 0 ) hdi.fmt |= HDF_SORTDOWN;
		::SendMessage( m_hHeader, HDM_SETITEM, 0, (LPARAM)&hdi );
	}

	::SendMessage( item(IDC_FILELIST), WM_SETREDRAW, TRUE, 0 );
	::InvalidateRect( item(IDC_FILELIST), NULL, TRUE );
}

void CArcViewDlg::DoRMenu(POINT pt)
{
	enum { RM_EXTRACTALL = 1, RM_EXTRACTSEL, RM_OPEN };

	HMENU m = ::CreatePopupMenu();
	::AppendMenu( m, MF_STRING, RM_EXTRACTALL, kiStr().loadRsrc(IDS_RM_EXTRACTALL) );
	::AppendMenu( m, MF_STRING, RM_EXTRACTSEL, kiStr().loadRsrc(IDS_RM_EXTRACTSEL) );
	::AppendMenu( m, MF_STRING, RM_OPEN,       kiStr().loadRsrc(IDS_RM_OPEN) );

	int id = ::TrackPopupMenu( m,
		TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,
		pt.x, pt.y, 0, hwnd(), NULL );
	::DestroyMenu( m );

	if( id==RM_EXTRACTALL || id==RM_EXTRACTSEL )
	{
		setdir();
		int result = ( id==RM_EXTRACTSEL )
			? m_pArc->melt( makeArcName(), m_ddir, &m_files )
			: m_pArc->melt( makeArcName(), m_ddir );
		reportMeltResult( result );
	}
	else if( id==RM_OPEN )
	{
		// m_files[].selected was already set by the NM_RCLICK handler.
		openSelected();
	}
}

void CArcViewDlg::rebuildContent()
{
	kiStr str;
	kiListView ctrl( this, IDC_FILELIST );

	//-- Clear previous content
	m_files.empty();
	m_fileIndices.empty();
	m_folderPaths.empty();
	m_folderRawlines.empty();
	m_folderSorted.empty();
	m_treeNodes.empty();
	m_rows.empty();
	m_iconExtCache.empty();
	m_iconIdxCache.empty();

	::SendMessage( item(IDC_FILELIST), LVM_DELETEALLITEMS, 0, 0 );
	while( ::SendMessage( item(IDC_FILELIST), LVM_DELETECOLUMN, 0, 0 ) );
	::SendMessage( m_hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT );

	//-- List archive content
	if( !m_pArc->list( makeArcName(), m_files ) || m_files.len()==0 )
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
		ctrl.insertColumn( 0, "Name", 160 );
		ctrl.insertColumn( 1, colHeader, 640 );

		// Only build the file-index side table here; ListView population is
		// deferred to FilterListByFolder (triggered by the initial tree select).
		for( unsigned int i=dataStart; i!=m_files.len(); i++ )
			if( m_files[i].isfile )
				m_fileIndices.add( i );
	}

	setOperationControls( m_bAble );

	//-- Folder tree
	{
		SHFILEINFO sfi; ::ZeroMemory( &sfi, sizeof(sfi) );
		SHFILEINFO fsi; ::ZeroMemory( &fsi, sizeof(fsi) );

		// Archive file icon (used for tree root node)
		kiPath path( m_arcBaseDir ); path += (const char*)m_arcShortName;
		::SHGetFileInfo( path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

		// Folder icon from system image list
		::SHGetFileInfo( "folder", FILE_ATTRIBUTE_DIRECTORY, &fsi, sizeof(fsi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES );
		m_folderIconIdx = fsi.iIcon;

		TVINSERTSTRUCT tvis;
		::ZeroMemory( &tvis, sizeof(tvis) );
		tvis.hParent      = TVI_ROOT;
		tvis.hInsertAfter = TVI_FIRST;
		tvis.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		kiPath rootLabel( (const char*)m_arcLongName );
		tvis.item.pszText  = const_cast<char*>( rootLabel.name() );
		tvis.item.lParam   = (LPARAM)-1;
		tvis.item.iImage         = sfi.iIcon;
		tvis.item.iSelectedImage = sfi.iIcon;
		HTREEITEM hAll = (HTREEITEM)::SendMessage( m_hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis );
		BuildFolderTree( hAll, fsi.iIcon );

		// Initialize m_folderRawlines as parallel to m_folderPaths (all empty).
		for( unsigned int _i = 0; _i < m_folderPaths.len(); _i++ )
			m_folderRawlines.add( kiStr() );

		// Drop standalone directory entries (e.g. 7z "D...." rows) whose name
		// already exists in the synthesized folder tree, to avoid showing the
		// same folder both as a file row and as a folder row.
		{
			unsigned int write = 0;
			for( unsigned int read = 0; read < m_fileIndices.len(); read++ )
			{
				const char* fn = m_files[ m_fileIndices[read] ].inf.szFileName;
				int fnLen = ::lstrlen( fn );
				bool isDirEntry = false;
				char key[MAX_PATH];
				bool found = false;
				unsigned int bisectPos = 0;
				if( fnLen > 0 && (fn[fnLen-1] == '/' || fn[fnLen-1] == '\\') )
				{
					if( fnLen < MAX_PATH )
					{
						::lstrcpy( key, fn );
						bisectPos = folder_bisect( m_folderPaths, m_folderSorted, key, &found );
					}
				}
				else if( fnLen > 0 && fnLen + 1 < MAX_PATH )
				{
					::lstrcpy( key, fn );
					key[fnLen]   = '\\';
					key[fnLen+1] = '\0';
					bisectPos = folder_bisect( m_folderPaths, m_folderSorted, key, &found );
					if( !found )
					{
						key[fnLen] = '/';
						bisectPos = folder_bisect( m_folderPaths, m_folderSorted, key, &found );
					}
				}
				isDirEntry = found;
				if( !isDirEntry )
					m_fileIndices[write++] = m_fileIndices[read];
				else
				{
					// Clear the selected/isfile union so this entry is not
					// passed to the archiver for extraction and is not hit
					// by the ShellExecute loop in the View handler.
					m_files[ m_fileIndices[read] ].selected = false;
					// Capture rawline for the folder row's info column.
					const char* raw = m_files[ m_fileIndices[read] ].rawline;
					if( raw[0] && bisectPos < m_folderSorted.len() )
					{
						unsigned int fi = m_folderSorted[bisectPos];
						if( fi < m_folderRawlines.len() )
							m_folderRawlines[fi] = kiStr( raw );
					}
				}
			}
			m_fileIndices.forcelen( write );
		}

		// Status bar reports the total file count (files + directory entries)
		{
			char tmp[255];
			kiStr full_filename = m_arcBaseDir + (const char*)m_arcLongName;
			wsprintf( tmp, kiStr().loadRsrc(IDS_ARCVIEW_MSG),
				m_fileIndices.len(),
				(const char*)m_pArc->arctype_name(full_filename)
			);
			sendMsgToItem( IDC_STATUSBAR, WM_SETTEXT, 0, (LPARAM)tmp );
		}

		// Reserve capacity so listrow pointers stored in LVITEM.lParam stay
		// stable across FilterListByFolder rebuilds.
		m_rows.alloc( m_fileIndices.len() + m_folderPaths.len() + 1 );
		::SendMessage( m_hTree, TVM_EXPAND,     TVE_EXPAND,  (LPARAM)hAll );
		::SendMessage( m_hTree, TVM_SELECTITEM, TVGN_CARET,  (LPARAM)hAll );
		// Explicit initial population (TVN_SELCHANGED during init may not
		// be forwarded to this dialog's proc).
		if( m_bAble ) FilterListByFolder( -1 );
	}
}

void CArcViewDlg::loadArchive( CArchiver* parc, const kiPath& base,
                                const char* sname, const char* lname )
{
	m_bLoading = true;
	::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
	::SetTimer( hwnd(), 1, 100, NULL );

	m_pArc         = parc;
	m_arcBaseDir   = base;
	m_arcLongName  = lname;
	m_arcShortName = sname;
	m_sortDir      = 0;
	m_curFolderIdx = -1;

	//-- Set base extraction directory; generate_dirname is applied at extraction time in setdir()
	{
		m_ddir = base;
		m_ddir.beBackSlash( true );
	}

	//-- Title: "Noah - filename"
	{ char _t[MAX_PATH+10]; wsprintf(_t, "Noah - %s", kiPath(lname).name()); sendMsg(WM_SETTEXT,0,(LPARAM)_t); }

	updateDdirDisplay();

	rebuildContent();

	::KillTimer( hwnd(), 1 );
	m_bLoading = false;
	::SetCursor( ::LoadCursor(NULL, IDC_ARROW) );
}

void CArcViewDlg::unloadArchive()
{
	m_pArc         = NULL;
	m_arcBaseDir   = "";
	m_arcLongName  = "";
	m_arcShortName = "";
	m_bAble        = false;

	//-- Clear data arrays
	m_files.empty();
	m_fileIndices.empty();
	m_folderPaths.empty();
	m_folderRawlines.empty();
	m_folderSorted.empty();
	m_treeNodes.empty();
	m_rows.empty();
	m_iconExtCache.empty();
	m_iconIdxCache.empty();

	//-- Clear list view (items and columns)
	::SendMessage( item(IDC_FILELIST), LVM_DELETEALLITEMS, 0, 0 );
	while( ::SendMessage( item(IDC_FILELIST), LVM_DELETECOLUMN, 0, 0 ) );

	//-- Clear tree
	::SendMessage( m_hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT );

	//-- Re-insert placeholder column
	kiListView ctrl( this, IDC_FILELIST );
	ctrl.insertColumn( 0, "", 510 );

	//-- Reset title and status
	sendMsg( WM_SETTEXT, 0, (LPARAM)"Noah" );
	sendMsgToItem( IDC_STATUSBAR, WM_SETTEXT, 0, (LPARAM)"" );

	//-- Disable operation controls
	setOperationControls( false );
}

void CArcViewDlg::doFileOpen()
{
	char buf[MAX_PATH] = "";
	OPENFILENAME ofn;
	::ZeroMemory( &ofn, sizeof(ofn) );
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = hwnd();
	ofn.lpstrFilter = "Archive Files\0*.zip;*.7z;*.rar;*.lzh;*.tar;*.gz;*.bz2;*.cab;*.arj\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile   = buf;
	ofn.nMaxFile    = MAX_PATH;
	ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if( !::GetOpenFileName(&ofn) ) return;
	openFromPath( buf );
}

void CArcViewDlg::openFromPath( const char* fullpath )
{
	CArchiver* parc = myarc().find_melter_for( kiPath(fullpath) );
	if( !parc )
	{
		char msg[MAX_PATH + 64];
		wsprintf( msg, "Not a recognized archive:\n%s", fullpath );
		app()->msgBox( msg );
		return;
	}
	kiPath shortPath( fullpath );
	shortPath.beShortPath();

	// Extract leaf name before beDirOnly modifies shortPath
	char sname[MAX_PATH];
	::lstrcpy( sname, kiPath::name( (const char*)shortPath ) );

	kiPath shortBase( shortPath );
	shortBase.beDirOnly();

	loadArchive( parc, shortBase, sname, kiPath::name(fullpath) );
	updateMRU( fullpath );
}

void CArcViewDlg::handleDroppedFile( const char* path )
{
	CArchiver* parc = myarc().find_melter_for( kiPath(path) );
	if( parc )
	{
		openFromPath( path );
	}
	else
	{
		// Not a recognized archive: let Noah compress it
		cCharArray files;
		files.add( path );
		myapp().do_files( files );
	}
}

void CArcViewDlg::updateMRU( const char* fullpath )
{
	// Copy path to a local buffer before modifying m_mruList: fullpath often
	// points directly into m_mruList[0].m_pBuf, which push_mru_entry's
	// left-shift would overwrite in-place, corrupting the value.
	char local[MAX_PATH];
	::lstrcpy( local, fullpath );
	// Re-read from INI so we don't overwrite entries added by other dialogs
	// or processes (rememberMRU) since this dialog was initialized.
	load_mru_list( m_mruList );
	push_mru_entry( m_mruList, local );
	persist_mru_list( m_mruList );
	rebuildMRUMenu();
}

void CArcViewDlg::rememberMRU( const char* fullpath )
{
	StrArray list;
	load_mru_list( list );
	push_mru_entry( list, fullpath );
	persist_mru_list( list );
}

void CArcViewDlg::rebuildMRUMenu()
{
	if( !m_hMenu ) return;
	HMENU hFile   = ::GetSubMenu( m_hMenu, 0 );   // "File" popup (index 0)
	HMENU hRecent = ::GetSubMenu( hFile,   1 );    // "Recent Files" popup (index 1)
	if( !hRecent ) return;
	// Clear all existing items
	while( ::GetMenuItemCount(hRecent) > 0 )
		::DeleteMenu( hRecent, 0, MF_BYPOSITION );
	if( m_mruList.len() == 0 )
	{
		::AppendMenu( hRecent, MF_STRING|MF_GRAYED, IDM_FILE_MRU_FIRST,
			kiStr().loadRsrc(IDS_MRU_NONE) );
	}
	else
	{
		for( unsigned int i=0; i<m_mruList.len(); i++ )
		{
			char label[MAX_PATH + 4];
			wsprintf( label, "&%d %s", i+1, (const char*)m_mruList[i] );
			::AppendMenu( hRecent, MF_STRING, IDM_FILE_MRU_FIRST+i, label );
		}
	}
	::DrawMenuBar( hwnd() );
}

void CArcViewDlg::applyToolbarVisibility()
{
	static const UINT items[] = {IDC_MELTEACH,IDC_SHOW,IDC_SEP1,IDC_SETTINGS,IDC_SEP2,IDC_DDIRLABEL,IDC_DDIR,IDC_REF};
	int sw = m_bShowToolbar ? SW_SHOW : SW_HIDE;
	for( int i=0; i<8; i++ )
		::ShowWindow( item(items[i]), sw );
	m_listTop = m_bShowToolbar ? m_topRowH : 0;

	RECT rc; ::GetClientRect( hwnd(), &rc );
	RECT sbar; ::GetClientRect( item(IDC_STATUSBAR), &sbar );
	int sbH = sbar.bottom - sbar.top;
	int paneH = rc.bottom - m_listTop - sbH;
	if( m_bShowToolbar ) LayoutTopRow( rc.right );
	LayoutPanes( rc.right, paneH );
	::SetWindowPos( item(IDC_STATUSBAR), NULL, 0,
		rc.bottom - sbH, 0, 0, SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	::InvalidateRect( hwnd(), NULL, TRUE );
}

void CArcViewDlg::applyTreeVisibility()
{
	::ShowWindow( m_hTree, m_bShowTree ? SW_SHOW : SW_HIDE );

	RECT rc; ::GetClientRect( hwnd(), &rc );
	RECT sbar; ::GetClientRect( item(IDC_STATUSBAR), &sbar );
	int sbH = sbar.bottom - sbar.top;
	int paneH = rc.bottom - m_listTop - sbH;
	LayoutPanes( rc.right, paneH );
	::InvalidateRect( hwnd(), NULL, TRUE );
}
